"""Compiled member-write checks; stock research UI and grants need a game canary."""
import json
import os
from pathlib import Path
import unreal

root = Path(os.environ['SMARTOUTLET_TEST_OUTPUT'])
manager_class = unreal.load_class(None, '/Game/Mods/SmartOutlet/ModActor.ModActor_C')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
manager = actors.spawn_actor_from_class(manager_class, unreal.Vector())
skill = unreal.new_object(unreal.VoyageSkill)
parent = unreal.new_object(unreal.VoyageSkill)
# Concrete stand-ins for unrelated references: only pointer preservation is tested.
quest = unreal.new_object(unreal.VoyageSkill)
requirement = unreal.new_object(unreal.VoyageSkill)
checks = []
def fields():
    return skill.get_editor_property('unlock')
def check_preserved():
    data = fields()
    assert data.get_editor_property('cost') == 2
    assert data.get_editor_property('parent_skill') == parent
    assert data.get_editor_property('quest_on_unlock') == quest
    assert list(data.get_editor_property('custom_requirements')) == [requirement]
try:
    data = fields()
    for key, value in [('unlock_method', unreal.VoyageSkillUnlockMethod.NEVER), ('requirement', 30),
                       ('cost', 2), ('parent_skill', parent), ('quest_on_unlock', quest),
                       ('custom_requirements', [requirement])]:
        data.set_editor_property(key, value)
    skill.set_editor_property('unlock', data)
    assert not manager.call_method('ApplyResearchTier', (None,))
    assert manager.call_method('ApplyResearchTier', (skill,))
    assert fields().get_editor_property('unlock_method') == unreal.VoyageSkillUnlockMethod.TIER
    # Stock fresh-water tank research: Tier 11 on Steam25982825.
    assert fields().get_editor_property('requirement') == 11
    check_preserved()
    checks.append('Only method and requirement change; cost and other fields survive')
    assert manager.call_method('ApplyResearchTier', (skill,))
    manager.call_method('RestoreResearchTier')
    assert fields().get_editor_property('unlock_method') == unreal.VoyageSkillUnlockMethod.NEVER
    assert fields().get_editor_property('requirement') == 30
    check_preserved()
    assert not manager.get_editor_property('ResearchApplied')
    checks.append('Repeated apply preserves the original values; cleanup restores them')
    manager.call_method('RestoreResearchTier')
    assert fields().get_editor_property('requirement') == 30
    checks.append('Repeated cleanup is harmless')
    assert manager.call_method('ApplyResearchTier', (skill,))
    data = fields()
    data.set_editor_property('requirement', 7)
    skill.set_editor_property('unlock', data)
    manager.call_method('RestoreResearchTier')
    assert fields().get_editor_property('requirement') == 7
    check_preserved()
    checks.append('Cleanup preserves a later change by another owner')
    # Default-disabled path does not load or mutate the stock technology.
    manager.call_method('StartResearchTier')
    assert not manager.get_editor_property('ResearchApplied')
    checks.append('Disabled configuration skips the runtime change')
finally:
    actors.destroy_actor(manager)
(root/'research-tier-validation.json').write_text(json.dumps({'status':'passed','checks':checks,'stockGameValidated':False}, indent=2)+'\n')
unreal.log('SmartOutlet research Tier checks passed: ' + str(len(checks)))
