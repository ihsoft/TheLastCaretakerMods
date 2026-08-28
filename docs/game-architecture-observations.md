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

Проверенный baseline хранится в
`mods/DonkLiftKeyboardControl/Scripts/main.lua`.

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
  `ue4ss/UE4SS_Signatures/FText_Constructor.lua` и требует повторной проверки
  после обновления exe.
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

## Извлеченные материалы

В `artifacts/` сохранены:

- `.usmap` для текущей сборки;
- JSON и псевдокод Blueprint'ов погрузчика, HUD и input hints;
- JSON всех найденных `InputMappingContext`;
- reflection-списки свойств нативных классов;
- отчеты инвентаризации пакетов и совпадений.

Крупные воспроизводимые промежуточные каталоги `artifacts/raw/` и
`artifacts/extracted/` исключены из Git.

## Правила следующих экспериментов

1. Рабочий control baseline не изменять одновременно с HUD-прототипом.
2. Каждый эксперимент делать отдельным коммитом или веткой поверх baseline.
3. Перед заменой файла проверять, что процесс игры завершен.
4. После теста фиксировать три результата отдельно: физика, HUD, стабильность.
5. При самопроизвольном газе/руле или падении сразу возвращаться на baseline по
   Git, не пытаться чинить поверх неизвестного состояния.
6. Для нового HUD предпочтителен отдельный overlay/UserWidget, читающий только
   `current_hud_throttle` и `current_hud_steering`; штатный HUD сначала не
   удалять, а лишь скрывать после успешной проверки копии.
