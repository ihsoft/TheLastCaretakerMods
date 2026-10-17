# ScopeFix architecture

The mod changes two game-authored aiming-camera offsets: one on the electric
rifle and one on the shared RedDot sight. Its builder extracts current stock
assets, checks the expected original values, changes only those offsets, and
packages one stock-override IoStore container. The RedDot adjustment can also
affect other compatible weapons; their behavior is not established here.

[Aiming configuration](docs/aiming-configuration.md) records the chosen values,
reasoning, and validation boundary. [README.md](README.md) owns the build and
installation interface.
