#!/usr/bin/env python3
"""Opt-in start/finish measurement for one explicitly named Codex JSONL session."""

from __future__ import annotations

import argparse
import collections
import datetime as dt
import hashlib
import json
import pathlib
import re
import sys
from typing import Any, Iterable


TOKEN_KEYS = ("input_tokens", "cached_input_tokens", "output_tokens", "reasoning_output_tokens")


def utc_now() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat().replace("+00:00", "Z")


def parse_time(value: str | None) -> dt.datetime | None:
    if not value:
        return None
    return dt.datetime.fromisoformat(value.replace("Z", "+00:00"))


def iter_records(path: pathlib.Path) -> Iterable[dict[str, Any]]:
    # A live file may end with a partially written JSON object. Complete earlier
    # records remain usable; malformed interior records are counted separately.
    with path.open("r", encoding="utf-8") as stream:
        for number, line in enumerate(stream, 1):
            if not line.endswith("\n"):
                try:
                    yield json.loads(line)
                except (json.JSONDecodeError, UnicodeError):
                    yield {"_incomplete_final_line": number}
                    return
                continue
            try:
                yield json.loads(line)
            except (json.JSONDecodeError, UnicodeError):
                yield {"_malformed_line": number}


def session_metadata(path: pathlib.Path) -> dict[str, Any]:
    for record in iter_records(path):
        if record.get("type") == "session_meta":
            payload = record.get("payload", {})
            return {key: payload.get(key) for key in ("id", "agent_path", "parent_thread_id", "timestamp", "cwd", "source")}
    raise ValueError(f"No complete session_meta record in {path}")


def token_total(record: dict[str, Any]) -> dict[str, int] | None:
    payload = record.get("payload", {})
    if record.get("type") != "event_msg" or payload.get("type") != "token_count":
        return None
    info = payload.get("info") or {}
    total = info.get("total_token_usage")
    return total if isinstance(total, dict) else None


def last_counter_before(path: pathlib.Path, cutoff: dt.datetime) -> tuple[str | None, dict[str, int] | None, int, bool]:
    found_at, found = None, None
    malformed, incomplete = 0, False
    for record in iter_records(path):
        if "_malformed_line" in record:
            malformed += 1
            continue
        if "_incomplete_final_line" in record:
            incomplete = True
            continue
        stamp = parse_time(record.get("timestamp"))
        if stamp and stamp > cutoff:
            continue
        total = token_total(record)
        if total is not None:
            found_at, found = record.get("timestamp"), total
    return found_at, found, malformed, incomplete


def latest_context_before(path: pathlib.Path, cutoff: dt.datetime) -> dict[str, Any] | None:
    found = None
    for record in iter_records(path):
        stamp = parse_time(record.get("timestamp"))
        if stamp and stamp <= cutoff and record.get("type") == "turn_context":
            payload = record.get("payload", {})
            found = {"at": record.get("timestamp")} | {key: payload.get(key) for key in ("model", "effort", "reasoning_effort", "service_tier")}
    return found


def ensure_tmp_output(path: pathlib.Path) -> pathlib.Path:
    repo = pathlib.Path(__file__).resolve().parents[1]
    tmp = (repo / "Tmp").resolve()
    resolved = path.resolve()
    if tmp not in resolved.parents:
        raise ValueError(f"Output directory must be below {tmp}")
    if resolved.exists():
        raise ValueError(f"Output directory must be new: {resolved}")
    resolved.mkdir(parents=True)
    return resolved


def ensure_tmp_contained(path: pathlib.Path) -> pathlib.Path:
    repo = pathlib.Path(__file__).resolve().parents[1]
    tmp = (repo / "Tmp").resolve()
    resolved = path.resolve()
    if tmp not in resolved.parents:
        raise ValueError(f"Measurement path must be below {tmp}")
    return resolved


def write_json(path: pathlib.Path, value: Any) -> None:
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def start(args: argparse.Namespace) -> int:
    session = pathlib.Path(args.session_path).resolve()
    if not session.is_file():
        raise ValueError(f"Session path is not a file: {session}")
    metadata = session_metadata(session)
    if metadata.get("id") != args.session_id:
        raise ValueError(f"Session ID mismatch: expected {args.session_id}, found {metadata.get('id')}")
    output = ensure_tmp_output(pathlib.Path(args.output_directory))
    if args.from_session_start:
        started = parse_time(metadata.get("timestamp"))
        if started is None:
            raise ValueError("Whole-session mode requires a valid session_meta timestamp.")
        counter_at = metadata["timestamp"]
        baseline = {key: 0 for key in TOKEN_KEYS}
        _, _, malformed, incomplete = last_counter_before(session, started)
        initial_context = None
    else:
        started = dt.datetime.now(dt.timezone.utc)
        counter_at, baseline, malformed, incomplete = last_counter_before(session, started)
        initial_context = latest_context_before(session, started)
    state = {
        "schema": 1,
        "status": "started",
        "session_path": str(session),
        "session_id": args.session_id,
        "started_at_utc": started.isoformat().replace("+00:00", "Z"),
        "session_size_at_start": session.stat().st_size,
        "baseline_counter_at": counter_at,
        "baseline_counter": baseline,
        "from_session_start": bool(args.from_session_start),
        "baseline_coverage": {"malformed_complete_lines": malformed, "incomplete_final_record": incomplete},
        "initial_model_context": initial_context,
        "metadata": metadata,
    }
    state_path = output / "measurement-start.json"
    write_json(state_path, state)
    print(json.dumps({"status": "started", "measurement_path": str(state_path), "session_id": args.session_id, "baseline_available": baseline is not None}, ensure_ascii=False))
    return 0


def bounded_delta(baseline: dict[str, int] | None, final: dict[str, int] | None, reset: bool) -> tuple[dict[str, int | None], bool]:
    if baseline is None or final is None:
        return {key: None for key in TOKEN_KEYS} | {"uncached_input_tokens": None}, reset
    if reset:
        return {key: None for key in TOKEN_KEYS} | {"uncached_input_tokens": None}, True
    delta: dict[str, int | None] = {}
    for key in TOKEN_KEYS:
        delta[key] = final[key] - baseline[key] if key in baseline and key in final else None
    cached, inputs = delta["cached_input_tokens"], delta["input_tokens"]
    delta["uncached_input_tokens"] = inputs - cached if inputs is not None and cached is not None else None
    return delta, False


def finish(args: argparse.Namespace) -> int:
    state_path = ensure_tmp_contained(pathlib.Path(args.measurement_path))
    if not state_path.is_file():
        raise ValueError(f"Measurement state is not a file: {state_path}")
    state = json.loads(state_path.read_text(encoding="utf-8-sig"))
    if state.get("status") != "started":
        raise ValueError("Measurement state is not an unfinished start record.")
    session = pathlib.Path(state["session_path"])
    metadata = session_metadata(session)
    if metadata.get("id") != state.get("session_id"):
        raise ValueError("Session identity changed since measurement start.")
    started = parse_time(state["started_at_utc"])
    cutoff = dt.datetime.now(dt.timezone.utc)
    usage_samples: list[dict[str, Any]] = []
    tool_calls: list[dict[str, Any]] = []
    malformed = 0
    incomplete = False
    compactions = 0
    contexts: list[dict[str, Any]] = [state["initial_model_context"]] if state.get("initial_model_context") else []
    last_seen_total: dict[str, int] | None = state.get("baseline_counter")
    reset = False
    for record in iter_records(session):
        if "_malformed_line" in record:
            malformed += 1
            continue
        if "_incomplete_final_line" in record:
            incomplete = True
            continue
        stamp = parse_time(record.get("timestamp"))
        if stamp is None or stamp <= started or stamp > cutoff:
            continue
        payload = record.get("payload", {})
        if record.get("type") == "turn_context":
            contexts.append({"at": record.get("timestamp")} | {key: payload.get(key) for key in ("model", "effort", "reasoning_effort", "service_tier")})
        if record.get("type") == "compacted":
            compactions += 1
        total = token_total(record)
        if total is not None:
            if last_seen_total is not None and any(key in last_seen_total and key in total and total[key] < last_seen_total[key] for key in TOKEN_KEYS):
                reset = True
            if total != last_seen_total:
                info = payload.get("info") or {}
                usage_samples.append({"at": record.get("timestamp"), "total": total, "request": info.get("last_token_usage")})
            last_seen_total = total
        if record.get("type") == "response_item" and payload.get("type") in ("function_call", "custom_tool_call"):
            serialized = payload.get("arguments", payload.get("input", ""))
            nested = re.findall(r"tools\.([A-Za-z0-9_]+)\s*\(", serialized) if isinstance(serialized, str) else []
            tool_calls.append({"at": record.get("timestamp"), "name": payload.get("name"), "nested_static_calls": nested})
    final = usage_samples[-1]["total"] if usage_samples else None
    delta, reset = bounded_delta(state.get("baseline_counter"), final, reset)
    used_tokens = delta["input_tokens"] + delta["output_tokens"] if delta["input_tokens"] is not None and delta["output_tokens"] is not None else None
    requests = [sample.get("request") for sample in usage_samples if isinstance(sample.get("request"), dict)]
    request_count = len(requests) if usage_samples and len(requests) == len(usage_samples) else None
    request_inputs = [request.get("input_tokens") for request in requests if isinstance(request.get("input_tokens"), int)]
    top_names = collections.Counter(call.get("name") or "<unknown>" for call in tool_calls)
    nested_names = collections.Counter(name for call in tool_calls for name in call["nested_static_calls"])
    interagent_names = {"send_message", "followup_task", "spawn_agent", "wait_agent", "interrupt_agent"}
    interagent_calls = sum(1 for call in tool_calls if (call.get("name") or "").split(".")[-1].split("__")[-1] in interagent_names)
    baseline_coverage = state.get("baseline_coverage") or {}
    session_truncated = session.stat().st_size < int(state.get("session_size_at_start", 0))
    incomplete_coverage = malformed > 0 or incomplete or int(baseline_coverage.get("malformed_complete_lines", 0)) > 0 or bool(baseline_coverage.get("incomplete_final_record")) or session_truncated
    usage_status = "counter_reset" if reset else "incomplete_coverage" if incomplete_coverage else "missing_baseline" if state.get("baseline_counter") is None else "measured" if usage_samples else "no_usage_samples"
    evidence = {
        "schema": 1,
        "status": "finished",
        "usage_status": usage_status,
        "session": metadata,
        "session_path": str(session),
        "measurement_start": state["started_at_utc"],
        "cutoff_utc": cutoff.isoformat().replace("+00:00", "Z"),
        "elapsed_seconds": (cutoff - started).total_seconds(),
        "baseline_counter": state.get("baseline_counter"),
        "final_counter": final,
        "tokens": delta,
        "used_tokens": used_tokens,
        "usage_sample_count": len(usage_samples),
        "request_count": request_count,
        "first_request_input_tokens": request_inputs[0] if request_inputs else None,
        "peak_request_input_tokens": max(request_inputs) if request_inputs else None,
        "tool_calls": {"top_level_count": len(tool_calls), "top_level_by_name": dict(top_names), "nested_static_count": sum(nested_names.values()), "nested_static_by_name": dict(nested_names)},
        "interagent_call_count": interagent_calls,
        "retry_count": None,
        "compactions": compactions,
        "model_contexts": contexts,
        "malformed_complete_lines": malformed,
        "coverage": {"baseline_available": state.get("baseline_counter") is not None, "baseline_scan": baseline_coverage, "finish_malformed_complete_lines": malformed, "finish_incomplete_final_record": incomplete, "session_truncated": session_truncated, "complete": not incomplete_coverage and state.get("baseline_counter") is not None},
        "notes": ["used_tokens is input_tokens plus output_tokens; cached input is counted fully and the reasoning subset is not added again.", "Reasoning tokens are an output subset.", "Cached input is an input subset.", "No usage samples is unknown usage, not zero.", "Tool levels are reported separately; nested calls are static matches inside orchestration calls.", "Inter-agent calls are counted, but message argument sizes are not reported as plaintext communication.", "Retries and their causes remain unknown without an explicit structured source."],
        "evidence": {"usage_samples": usage_samples, "tool_timeline": tool_calls},
    }
    output = state_path.parent
    json_path = output / "measurement-result.json"
    report_path = output / "measurement-report.txt"
    write_json(json_path, evidence)
    lines = [
        f"status: {usage_status}",
        f"used_tokens: {used_tokens if used_tokens is not None else 'unknown'} (input + output; cached input included; reasoning not added)",
        f"session: {metadata.get('id')}",
        f"window: {state['started_at_utc']} .. {evidence['cutoff_utc']} ({evidence['elapsed_seconds']:.1f}s)",
        "tokens: " + ", ".join(f"{key}={value if value is not None else 'unknown'}" for key, value in delta.items()),
        f"requests: {evidence['request_count'] if evidence['request_count'] is not None else 'unknown'}; first_input={evidence['first_request_input_tokens'] if evidence['first_request_input_tokens'] is not None else 'unknown'}; peak_input={evidence['peak_request_input_tokens'] if evidence['peak_request_input_tokens'] is not None else 'unknown'}",
        f"tools: top_level={len(tool_calls)}; nested_static={sum(nested_names.values())}",
        f"evidence: {json_path}",
    ]
    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    state["status"] = "finished"
    state["cutoff_utc"] = evidence["cutoff_utc"]
    state["result_path"] = str(json_path)
    state["report_path"] = str(report_path)
    write_json(state_path, state)
    print(json.dumps({"status": usage_status, "result_path": str(json_path), "report_path": str(report_path), "used_tokens": used_tokens, "tokens": delta, "requests": evidence["request_count"], "tool_calls": evidence["tool_calls"], "elapsed_seconds": evidence["elapsed_seconds"]}, ensure_ascii=False))
    return 0


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(description=__doc__)
    commands = result.add_subparsers(dest="command", required=True)
    begin = commands.add_parser("start")
    begin.add_argument("--session-path", required=True)
    begin.add_argument("--session-id", required=True)
    begin.add_argument("--output-directory", required=True)
    begin.add_argument("--from-session-start", action="store_true", help="Use the verified session_meta timestamp and an explicit zero baseline.")
    end = commands.add_parser("finish")
    end.add_argument("--measurement-path", required=True)
    return result


def main() -> int:
    args = parser().parse_args()
    return start(args) if args.command == "start" else finish(args)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as error:
        print(json.dumps({"status": "error", "error": str(error)}, ensure_ascii=False), file=sys.stderr)
        raise SystemExit(2)
