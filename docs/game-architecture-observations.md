# The Last Caretaker: architecture observations

Состояние исследования на 2026-08-27. Документ отделяет подтвержденные
наблюдения от рабочих гипотез. Основной исследованный объект — погрузчик
DonkLift-320 и его HUD.

## Среда и размещение

- Внутреннее имя игры и корневого модуля: `Voyage`.
- Установка игры: `P:\SteamLibrary\steamapps\common\Voyage`.
- Движок по данным проекта и mappings: Unreal Engine 5.7 (mapping-файл имеет
  версию 5.7.4).
- Загрузчик модов: UE4SS Compact 3.0.1 beta.
- Lua-моды загружаются из
  `Voyage\Binaries\Win64\ue4ss\Mods\<ModName>\Scripts\main.lua`.
- Исследовательский репозиторий:
  `R:\Codex\TheLastCaretakerMods`.

## Слои управления транспортом

Наблюдаемая цепочка имеет четыре отдельных слоя:

1. Enhanced Input преобразует физические клавиши или оси контроллера в
   `InputAction`.
2. Нативный класс `VoyageVehicleForkliftPawn` принимает действия и хранит
   необработанные поля `ThrottleInput` и `SteeringInput`.
3. Blueprint `BP_Forklift_Possesable_C` использует собственные рабочие значения
   `Acceleration` и `Steering`, от которых фактически движется погрузчик.
4. HUD получает отдельные обновления через `VoyageInGameVehicleWidget` и не
   обязан отображать текущие значения `Acceleration`/`Steering`.

Это разделение подтверждено экспериментально: можно получить правильную физику
при неизменной цифровой индикации HUD либо правильную индикацию от внедренного
Enhanced Input при неправильной физике.

## Классы погрузчика

### Blueprint-актор

- Класс:
  `/Game/Blueprints/Vehicles/BP_Forklift_Possesable.BP_Forklift_Possesable_C`.
- Рабочие Blueprint-поля, проверенные записью во время игры:
  - `Acceleration` — фактическая команда тяги;
  - `Steering` — фактическая команда руля.
- Запись плавных значений в эти поля каждый тик дает устойчивое аналоговое
  управление с клавиатуры.

### Нативный базовый класс

`VoyageVehicleForkliftPawn : VoyageVehiclePawn` содержит, среди прочего:

- настройки движения: `ThrottleForce`, `MaxSpeed`, `MaxSpeedReverse`,
  `ThrottleSpeedReduction`, `MaxSteerSpeed`;
- ссылки на отдельные действия вперед/назад и вправо/влево;
- `ThrottleInput`, `SteeringInput`, `ForkInput`, `ForkTiltInput`;
- состояние вил: `ForkOffset`, `ForkTiltOffset`;
- делегаты изменения вил.

Полезные нативные функции:

- `/Script/Voyage.VoyageVehicleForkliftPawn:GetThrottleInput`;
- `/Script/Voyage.VoyageVehicleForkliftPawn:GetSteeringInput`;
- `/Script/Voyage.VoyageVehicleForkliftPawn:SetThrottleInput`;
- `/Script/Voyage.VoyageVehicleForkliftPawn:SetSteeringInput`.

Базовый `VoyageVehiclePawn : Pawn` содержит `InputControls`,
`DynamicPlayerInputComponent`, компоненты камеры и меша, состояние possession,
ссылку на водителя, пробег и действие клаксона. Это указывает, что ввод
транспорта подключается динамически при посадке, а не является постоянной частью
обычного персонажа.

## Enhanced Input

Извлечены контексты:

- `/Game/Game/Input/Vehicle/IMC_Forklift_Keyboard`;
- `/Game/Game/Input/Vehicle/IMC_Forklift_Gamepad`;
- `/Game/Game/Input/Vehicle/DA_Input_Context_Forklift`.

Клавиатурный контекст использует четыре раздельных цифровых действия:

- `W` -> `IA_VehicleForward`;
- `S` -> `IA_VehicleBackward`;
- `D` -> `IA_VehicleRight`;
- `A` -> `IA_VehicleLeft`.

Контекст контроллера использует аналоговые оси:

- `Gamepad_LeftY` -> `IA_VehicleForward`;
- `Gamepad_LeftX` -> `IA_VehicleRight`.

На осях контроллера применен `InputModifierDeadZone` с нижним порогом `0.25`
и типом `Axial`. Именно эта ветка способна штатно переносить дробные значения.

Проверенный эксперимент: внедрение значения `0.30` через
`InjectInputVectorForAction` показало `30%` на HUD и заставило погрузчик ехать.
Следовательно, аналоговая цепочка Enhanced Input -> pawn -> HUD существует и
работает. Однако непрерывное внедрение из Lua оставляло действие активным и
приводило к самопроизвольному газу и повороту после отпускания клавиш. Пока не
найден надежный способ передавать отпускание/нулевое состояние в правильной фазе
Enhanced Input, этот путь нельзя считать безопасным.

## Рабочая модель клавиатурного мода

Проверенный исторический UE4SS baseline хранится в
`mods/DonkLiftKeyboardControlUE4SS/Scripts/main.lua`.

Архитектура baseline:

- `RegisterKeyBind` отслеживает `W/S/A/D`, а хуки нативных setter-функций
  фиксируют фактическое направление цифровой команды;
- для каждого управляемого актера хранится собственное состояние газа, руля и
  скорости изменения руля;
- обновление выполняется post-hook'ом тика
  `BP_Forklift_Possesable_C:ReceiveTick`;
- мод пишет сглаженные значения в `Acceleration` и `Steering`;
- `ThrottleInput` и `SteeringInput` после обработки обнуляются, чтобы штатная
  цифровая команда не перетирала сглаженное состояние;
- хуки getter-функций изолируют рабочее состояние от остаточных штатных данных;
- `X` плавно возвращает газ к нулю из обоих направлений;
- `C` плавно возвращает руль в центр;
- `F7` включает/выключает сглаживание для диагностики.

Параметры baseline:

- нарастание газа: `0.45` единицы в секунду;
- торможение/возврат газа: `0.80`;
- максимальная скорость изменения руля: `0.80`;
- ускорение изменения руля: `1.25`;
- торможение при реверсе руля: `2.50`;
- ограничение `DeltaTime`: `0.10`;
- порог цифрового направления: `0.50`.

SHA-256 проверенного baseline-файла:
`659D85E9931841E12299C370BCFB76677C7E0238B828734BD08AA3FB65C8BA9E`.

## HUD погрузчика

### Иерархия классов

- Blueprint:
  `/Game/UI/Game/HUD/BP_VoyageIngameForklift.BP_VoyageIngameForklift_C`;
- нативный класс `VoyageIngameForkliftWidget`;
- базовый нативный класс `VoyageInGameVehicleWidget`;
- далее общий `VoyageInGameWidget`/UMG.

`VoyageInGameVehicleWidget` хранит ссылки на:

- `SpeedTb`, `SpeedSlider`, `SpeedRadialSlider`;
- `ThrottleTB`, `ThrottleAngleOL`, `ThrottleSlider`,
  `ThrottleRadialSlider`;
- `SteeringTB`, `SteeringAngleOL`, `SteeringSlider`,
  `SteeringRadialSlider`;
- элементы оборотов, пробега, мощности и батареи;
- диапазоны углов и флаги форматирования текста.

`VoyageIngameForkliftWidget` добавляет ресурс/топливо, изменение ресурса,
панель клаксона, панель вил и значение вил.

В реально показанном HUD погрузчика найдены:

- вертикальный `ThrottleSlider` слева от спидометра;
- горизонтальный `SteeringSlider` снизу;
- `ThrottleTB` и `SteeringTB` с числовыми значениями;
- контейнеры `SizeBox`, в которых слайдеры являются единственными дочерними
  элементами.

Нативные точки обновления:

- `/Script/Voyage.VoyageInGameVehicleWidget:OnUpdateThrottle`;
- `/Script/Voyage.VoyageInGameVehicleWidget:OnUpdateSteering`.

### Подтвержденное поведение HUD

- Штатная клавиатура дает HUD крайние значения при нажатии и ноль при
  отпускании, независимо от сохраненных модом `Acceleration` и `Steering`.
- Вызов `OnUpdateThrottle`/`OnUpdateSteering` с подменой аргумента сам по себе не
  гарантирует изменение видимых элементов.
- Перехват `USlider:SetValue`, запись свойства `Value`, очистка `ValueDelegate`
  и прямой поиск виджетов по имени давали нестабильный либо нулевой результат.
- Некоторые комбинации перехвата getter/setter/HUD создавали обратную связь:
  после примерно 45% руль сам доворачивался до максимума; аналогично газ мог
  переходить с 50% на 100%. После удаления HUD-вмешательства этот эффект исчез.
- Создание нового `Slider` через `StaticConstructObject` успешно: копия появилась
  на месте штатного элемента. Но Blueprint продолжил обращаться к исходному
  объекту, и копия оставалась на нуле. Ее внешний вид также не полностью совпал
  со штатным, что показывает неполное копирование UMG style/slot state.
- Попытка прямо обновлять копию и числовой текст через
  `KismetTextLibrary:Conv_IntToText` завершилась падением игры. Этот вариант
  удален из baseline.

Вывод: виджет нельзя безопасно заменить одной записью
`hud.SteeringSlider = replacement`. Blueprint/UMG сохраняет другие ссылки или
скомпилированный доступ к исходному `WidgetTree`. Для собственного HUD лучше
создать независимый `UserWidget` верхнего уровня либо отдельный overlay, а
штатные элементы только скрыть. Такой эксперимент должен жить в отдельной ветке
или отдельном модуле и не менять цепочку управления.

## Подсказки клавиш

Исследованы `VoyageButtonInfoContainerWidget`, `VoyageButtonInfoWidget` и
Blueprint-варианты `BP_ButtonInfo*`.

- Видимые `E/H` находятся не в `KeybindRoot`, а в
  `BP_DynamicPlayerInputHorizontalWidget_Bottom.ContextInputActionsRoot`.
  `KeybindRoot` существует, но его вложенный action-контейнер пуст в runtime.
- Два новых `WBP_InteractIndicator_C` безопасно создаются через
  `WidgetBlueprintLibrary:Create` и добавляются в тот же горизонтальный root.
  Их именованные дочерние TextBlock доступны не сразу после `Create`, поэтому
  настройка выполняется на следующем секундном проходе.
- У копий отключается `bAutoUpdateKeyRebindings`, иначе нативное обновление может
  вернуть содержимое шаблона `E / Interact`.
- Причиной прежнего падения был не UMG, а отсутствующий адрес
  `FText(FString&&)` в UE4SS. Для Voyage 5.7.4 найдена и статически проверена
  уникальная сигнатура; она хранится в
  `mods/DonkLiftKeyboardControlUE4SS/UE4SS_Signatures/FText_Constructor.lua` и
  требует повторной проверки после обновления exe.
- Реализованы и проверены подсказки `X / Brake` и `C / Center`.
- Стандартный `GetCurrentLanguage()` возвращал `en` даже при русском интерфейсе.
  Источник истины игры — живой
  `VoyageGameUserSettings.CustomSettings.LanguageType`. Значение читается один
  раз при появлении HUD и кэшируется только до его уничтожения. Это позволяет
  подхватить язык, измененный в главном меню, при следующей загрузке игры.
- Проверены английские `Brake / Center` и русские `Тормоз / Выровнять`;
  остальные языки пока используют английский fallback.
- Финальная поставка — один `DonkLiftKeyboardControl`: физическое управление и
  подсказки имеют независимые циклы и состояние внутри одного Lua-файла. HUD не
  пишет `ThrottleInput`/`SteeringInput`, но отдельного пользовательского мода
  для подсказок нет, поскольку без управления он не имеет самостоятельного
  смысла.
- Диагностические `Probe`/`Diagnostics`-моды удалены после завершения
  эксперимента. Повторно создавать их следует только во временной ветке;
  переиспользуемые инструменты исследования хранятся в `tools/`.

## Автономный IoStore-мод

### Повторная проверка для build 25056839

Для Steam build `25056839`, game UE `5.8.1`, executable SHA-256
`CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`
актуальные mappings и stock exports подтверждают прежнюю иерархию
`VoyageVehicleForkliftPawn : VoyageVehiclePawn`, поля `SteeringInput` и
`ThrottleInput`, native action identities и все пятнадцать записей
`IMC_Forklift_Keyboard`. Значения HUD enum и `Russian = 11` также не
изменились. В `FPlayerInputInterfaceAction` появились шесть delegate-полей в
хвосте; первые семнадцать полей, используемые сгенерированным графом, сохранили
имена, типы и порядок.

Для новой версии мод генерируется установленным UE Editor `5.8.2`. Cook обязан
использовать `-SkipZenStore`, а упаковка — канонический patched retoc с его
явным compatibility profile `UE5_7`.

Тестовый контейнер для build `25056839` прошёл свежую extraction исходного
погрузчика и `scriptobjects.bin`, точные relocation assertions, генерацию,
loose cook, шестипакетный inventory, `retoc verify`, установку и сверку хэшей.
Независимый разбор уже упакованной подмены подтвердил parent
`/Game/Mods/DonkLiftKeyboard/BP_Forklift_Original`, helper
`/Game/Mods/DonkLiftKeyboardControl/ModActor`, native owner
`VoyageVehiclePawn:GetProvidedActionsBP`, размер action struct `160`,
`Russian = 11` и HUD type `Central = 0`. Это подтверждение контейнера, но не
замена runtime-проверке управления и HUD. Пользователь затем подтвердил в игре
`5.5`, что управление, native percentage, подсказки X/C, пауза, выход и
повторная посадка работают; этот пакет стал checkpoint мода `v2`.

Версия мода не повторяет версию игры. `v1` известна как несовместимая с игрой
`5.5` преимущественно из-за перехода с UE `5.7.4` на `5.8.1`, однако будущая
смена версии игры сама по себе не доказывает несовместимость. Версия игры,
Steam build и UE сохраняются как проверенная матрица совместимости и provenance,
а не как runtime allowlist.

Текущий исходник мода находится в `mods/DonkLiftKeyboardControl`. Это
editor-only Unreal Engine 5.8.2 project: C++-классы внутри него являются
минимальными зеркалами `/Script/Voyage`, необходимыми только для компиляции
Blueprint. Они не входят в поставляемый контейнер; runtime использует нативные
классы игры.

Один `DonkLiftKeyboardControl_P` содержит одну прозрачную подмену package path:

1. Полный оригинальный `BP_Forklift_Possesable` перемещается из
   `/Game/Blueprints/Vehicles/BP_Forklift_Possesable` в равнодлинный
   `/Game/Mods/DonkLiftKeyboard/BP_Forklift_Original`. На исходном пути лежит
   child Blueprint: он сохраняет штатное поведение, устанавливает helper actor
   и расширяет список стандартных действий.
Равная длина нужна потому, что builder делает проверенную замену package name
в cooked bytes без изменения таблиц размеров. Перемещённый оригинал — снимок
установленной версии игры. Он не коммитится и перед каждой сборкой должен
заново извлекаться из текущего IoStore; следовательно, обновления оригинального
погрузчика не попадут в старый собранный мод автоматически.

Генераторы создают ровно пять собственных packages: helper actor, два
`VoyageInputAction`, замену `IMC_Forklift_Keyboard` и child погрузчика. Builder
объединяет их, свежий оригинал погрузчика и `scriptobjects.bin` в один
шести-asset контейнер. UE4SS и DML на runtime не нужны.

### Управление автономного helper

- Каждый helper получает только собственный погрузчик через
  `GetParentActor()` дочернего актора. Интеграция выполняется лишь при
  `IsPlayerControlled()`. Глобальный `GetPlayerPawn(0)` здесь неверен: после
  выхода он теряет ссылку на покинутый погрузчик, а helpers нескольких
  экземпляров могут одновременно писать в один активный pawn.
- Игра пишет точные цифровые команды `-1`, `0`, `1` в `ThrottleInput` и
  `SteeringInput`. Helper интегрирует собственное аналоговое состояние по
  `DeltaSeconds` и записывает результат обратно в те же поля; поэтому одно
  значение одновременно управляет физикой и штатной процентной индикацией.
- Собственное активное значение ограничено `±0.9999`, чтобы не совпадать с
  цифровыми маркерами; HUD округляет его до `100%`. В Blueprint-варианте
  исторический Lua-sentinel `0.0001` не требуется.
- Газ меняется на треть полного диапазона в секунду. Для руля подтверждены
  maximum speed `1.20`, acceleration `2.50`, reversal braking `5.00`.
- `X` немедленно обнуляет газ. `C` немедленно обнуляет руль и скорость его
  изменения. На каждом тике неуправляемого родительского погрузчика helper
  пишет честный `0` в оба native input и очищает три внутренних состояния.
  Уже повёрнутые колёса при этом могут визуально остаться повёрнутыми до
  следующей посадки: их pose хранится отдельно от активного рулевого ввода.

### Нативный producer стандартных действий

`VoyagePlayerInputInterfaces.GetProvidedActions()` возвращает общий массив
`FPlayerInputInterfaceAction`. Нативный vehicle producer добавляет штатные
действия, затем вызывает BlueprintNativeEvent
`VoyageVehiclePawn.GetProvidedActionsBP()`. Точный owner функции — базовый
`VoyageVehiclePawn`, а не `VoyageVehicleForkliftPawn`: same-name функция на
неверном stand-in компилируется, но не является override. После исправления
editor-only иерархии call-marker завершил игру, а следующий override успешно
добавил `X / Brake` и `C / Center` в штатную нижнюю панель.

Shipping enum `EPlayerInputInterfaceActionType` содержит ровно `Central=0`,
`Context=1`, `Hidden=2`. Нижняя E/H-панель фильтрует `Central`, центральная
F/R-панель — `Context`. Прямой тест с отключением фильтра нижней панели вывел
в ней обе группы, подтвердив общий upstream-массив и независимую фильтрацию.

В оригинальном HUD обе панели используют зарегистрированный
`VoyageDynamicPlayerInputComponent`, а не owner-ветку:
`bRegisterFromWidgetOwner=false`. Его `InputComp` —
`EnhancedInputComponent` по смещению `0x250` текущего executable;
`VoyageLocatorSubsystem` хранит weak-массив таких компонентов.

`InteractiveInterface.GetInteractiveProvidedActions(Character, Component)`
отвечает за внешнее взаимодействие персонажа с объектом и не является
producer-ом водительской панели.

### Рендеринг и порядок подсказок

`BP_DynamicPlayerInputHorizontalWidget_Bottom.ContextInputActionsRoot` —
подтверждённый live host нижнего ряда. Native update дедуплицирует полные
`FPlayerInputInterfaceAction` через `TSet`, после чего контейнер обходит sparse
slots. Поэтому порядок returned array и одинаковый `Priority` не задают
визуальный порядок; первоначально X/C появились как `C E H X`.

Экспериментальный child HUD мог найти собственные widgets по точной UObject
identity и переставить их в конец. Однако ESC/resume повторно заполняет тот же
ряд без реконструкции owning HUD: при входе получалось `E H X C`, после меню —
нативное `C E H X`. Периодический опрос ради косметического порядка признан
неоправданным. Production-мод не подменяет HUD и принимает порядок, который
выбирает игра; стандартные actions и их lifecycle при этом остаются нативными.

Локализация берётся из
`VoyageGameUserSettings.CustomSettings.LanguageType`, а не из
`KismetInternationalizationLibrary.GetCurrentLanguage`. Подтверждены
`English=1` и `Russian=11`; строки — `Brake / Center` и
`Тормоз / Выровнять`, остальные языки используют английский fallback.

### Версионные нативные данные

Для Steam build `23962331`, executable SHA-256
`6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D`:

- forklift action producer начинается по VA `0x1451C2830`, вызывает base
  producer `0x1451C5C10`; generated-event thunk `GetProvidedActionsBP` —
  `0x1450A7BE0`;
- `FPlayerInputInterfaceAction` занимает `0x230` bytes;
- offsets action-полей: Horn `0x3E0`, ForkTiltUp `0x4C0`, ForkTiltDown
  `0x4C8`, ForkUp `0x4D0`, ForkDown `0x4D8`, ThrottleForward `0x4E0`,
  ThrottleBackward `0x4E8`, SteeringRight `0x4F0`, SteeringLeft `0x4F8`,
  Exit `0x500`.

Эти адреса, offsets и editor-only зеркала нужно считать недействительными
после изменения fingerprint executable до повторного анализа.

### Сборка

Шесть production assets готовятся отдельными
`-CookSinglePackageNoRefs`-запусками через
`mods/DonkLiftKeyboardControl/Cook-DonkLiftAssets.ps1`. Broad `CookDir` для
этого проекта непригоден: он следует editor dependencies, начинает готовить
глобальные shaders и посторонние Engine/OpenWorld assets. Builder
`Build-InheritancePackage.ps1` проверяет обе равнодлинные подмены, собирает один
IoStore-контейнер и запускает `retoc verify`.

В этой среде UnrealBuildTool запускается с `-NoUBA`, а commandlets — с
`-ddc=NoZenLocalFallback` и workspace-local `-LocalDataCachePath`, иначе Zen
пытается стартовать с недоступным data directory.

## Ограничения Lua/UE4SS, найденные экспериментально

- `pcall` ловит Lua-ошибки, но не защищает от падения внутри нативного marshaling
  или Unreal-функции.
- Передача таблицы `{ KeyName = "W" }` как `FKey` в
  `PlayerController:IsInputKeyDown` вызвала нативное падение. Для подобных API
  нужен настоящий `FKey`, полученный из существующего mapping-объекта, либо иной
  безопасный источник.
- Внедренный Enhanced Input может пережить Lua-команду, поэтому отсутствие
  очередного ненулевого вызова не эквивалентно отпусканию оси.
- Замена UObject-поля виджета не означает, что скомпилированный Blueprint начнет
  использовать новый объект.
- Любые изменения одновременно в физическом input path и HUD path затрудняют
  диагностику и могут создать обратную связь. Их следует испытывать раздельно.

## Воспроизведение наблюдений

Снимки ресурсов игры не являются частью репозитория. Для текущей установленной
версии они заново создаются в игнорируемом `artifacts/`:

- `tools/Get-VoyageBuildFingerprint.ps1` фиксирует версию и SHA-256;
- `tools/Extract-VoyagePackage.ps1` извлекает узкий package filter через
  `retoc`;
- `tools/Inspect-VoyageAsset.ps1` получает JSON, псевдокод и reflection через
  `VoyageAssetInspector`/CUE4Parse;
- `tools/VoyageMappingsDumper` создаёт свежий mapping при временно подключённом
  UE4SS.

Точный порядок описан в `docs/research-workflow.md`. После обновления игры
старые локальные результаты нельзя использовать как подтверждение без нового
fingerprint и повторного извлечения.

## Правила следующих экспериментов

1. Рабочий control baseline не изменять одновременно с HUD-прототипом.
2. Каждый эксперимент делать отдельным коммитом или веткой поверх baseline.
3. Перед заменой файла проверять, что процесс игры завершен.
4. После теста фиксировать три результата отдельно: физика, HUD, стабильность.
5. При самопроизвольном газе/руле или падении сразу возвращаться на baseline по
   Git, не пытаться чинить поверх неизвестного состояния.
6. Отдельный overlay допустим как сильный call-marker, но не как финальная
   реализация стандартных подсказок. Финальный путь должен входить в native
   dynamic-input lifecycle и скрываться вместе со штатным HUD.
7. После одного-двух no-op не менять очередные поля вслепую: усилить маркер или
   пересмотреть producer/consumer/owner chain.
