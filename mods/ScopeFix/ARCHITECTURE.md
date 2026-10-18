# ScopeFix architecture

The mod changes two game-authored aiming-camera offsets: one on the electric
rifle and one on the shared RedDot sight. Its builder extracts current stock
assets, checks the expected original values, changes only those offsets, and
packages one stock-override IoStore container. The RedDot adjustment can also
affect other compatible weapons; their behavior is not established here.

The offsets prioritize long-range alignment; shots can land below the aim
point at close range. Weapon animation limits precise impact assessment, so
the alignment is qualitative rather than a measured accuracy guarantee.
Other sight assets are unchanged.

[Aiming configuration](docs/aiming-configuration.md) records the chosen values,
reasoning, and validation boundary.
