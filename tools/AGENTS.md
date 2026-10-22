# Tools index

For normal tool use, start with [tool index](tool-index.md), choose the task,
and run the documented public entry point as a black box. The
[tool catalog](tool-catalog.md) maps exact operations to scripts and focused
guides.

- [Toolchain maintenance](toolchain-maintenance.md): repair a published binary
  or deliberately change a tool implementation.
- [Toolchain validation](toolchain-validation.md): diagnose an unexpected native
  result or validate a toolchain change.
- [Git coordination](repository-git-coordination.md): required path for any Git
  mutation performed while maintaining tools.

When a tool source directory has its own `AGENTS.md`, use it as the local index.
README files are short folder labels and are not agent guidance. Open source
only for explicit tool development or after the documented black-box path fails
or produces an unexpected result.
