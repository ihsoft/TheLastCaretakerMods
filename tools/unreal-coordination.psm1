function Invoke-UnrealQueueEntry {
    param([string]$ScriptPath, [System.Collections.IDictionary]$Parameters, [string]$Operation)
    # A synchronous child of the queue wrapper shares its parent's reservation.
    if($env:CODEX_UNREAL_QUEUE_LEASE){throw 'Queue entry wrapper cannot recursively launch itself'}
    $arguments=@('-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',$ScriptPath)
    foreach($key in $Parameters.Keys){
        $value=$Parameters[$key]
        if($value -is [System.Management.Automation.SwitchParameter]){
            if($value.IsPresent){$arguments+='-'+$key}
        }else{
            $arguments+='-'+$key
            $arguments+=@($value | ForEach-Object {[string]$_})
        }
    }
    & (Join-Path $PSScriptRoot 'Invoke-UnrealQueued.ps1') -Operation $Operation -FilePath 'powershell.exe' -ArgumentList $arguments
}
Export-ModuleMember -Function Invoke-UnrealQueueEntry
