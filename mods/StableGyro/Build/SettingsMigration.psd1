@{
    RetiredKeys = @('AltitudeStabilizationDelaySeconds','CompensateTiltLift',
        'TiltLiftCompensationMultiplier','StabilizationMinimumRotorSpeedPercent',
        'EnableFuelDiagnostics','FuelConsumptionPerMinute')
    RetiredComments = @(
        '# Absolute delay after releasing Space at full throttle before exact altitude hold activates.',
        '# A release below full throttle is ignored. Set to 0 for immediate activation. Clamped to 0..60 seconds.',
        '# Keep vertical rotor lift while pitch is tilted forward/back.',
        '# Scale the separate world-up force: 0=none, 1=lost vertical component. Clamped to 0..5.',
        '# Percent of the stock rotor''s physical maximum speed (0 disables the gate).',
        '# Percent of the stock rotor''s normal 100% operating speed (0 disables the gate).',
        '# Temporary compact fuel/throttle diagnostics; set false to hide on the next Gyro entry.',
        '# Valid range is 0.5..2.5; missing or invalid values fall back to 1.0.',
        '# Base Petrol request at 100% engine throttle, in native resource units per minute.'
    )
    DefaultsComment = '# Defaults added by a newer StableGyro build.'
}
