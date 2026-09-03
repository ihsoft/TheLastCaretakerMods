# Журнал исследовательских граблей

Здесь фиксируются подходы, которые выглядели правдоподобно, но дали падение,
no-op или неверную архитектуру. Цель — не повторять их после обновления игры
или context compaction.

## Общие

- Успешная компиляция/cook доказывает только корректность editor-стенда, но не
  то, что runtime вызывает созданный граф. Для каждого нового callback нужен
  различимый call-marker и проверка в игре.
- Несколько последовательных no-op означают, что, вероятнее всего, неверна
  цепочка владения или producer, а не очередное поле структуры. После одного
  или двух no-op нужно усиливать маркер либо возвращаться к архитектуре.
- Не менять управление и HUD в одном эксперименте. Такая смесь уже приводила
  к самостоятельному газу/рулю и делала причину неразличимой.
- Не заменять файлы установленного мода при запущенной игре.
- Не хранить извлечённый снимок игры как знание. Без fingerprint он быстро
  становится правдоподобной, но устаревшей ловушкой.

## UObject и Blueprint

- Разные Lua-wrapper-объекты могут представлять один UObject. Сравнение wrapper
  identity не является стабильной идентичностью актора.
- Для child Blueprint с unversioned properties недостаточно отразить в
  editor-mirror только поле, к которому обращается новый граф. Cook сериализует
  унаследованный CDO по индексам схемы editor-класса, а runtime читает его по
  полной shipping-схеме. Неполный mirror может успешно скомпилироваться,
  приготовиться и пройти `retoc verify`, но упасть ещё при async loading с
  `Bad export index` до вызова любого графа. Так был отклонён Boat HUD child:
  `BP_VoyageIngameBoatHud.Default__BP_VoyageIngameBoatHud_C`, индекс
  `66559/5`. Перед runtime-тестом нужен exact native property prefix и
  отдельная проверка cooked CDO против shipping mapping.
- Запись нового UObject в поле виджета не гарантирует, что скомпилированный
  Blueprint или `WidgetTree` начнёт использовать эту ссылку.
- Копия оригинального Blueprint под новым package path сохраняет поведение, но
  это snapshot текущей игры. Её нужно извлекать заново для каждой версии, а не
  коммитить в репозиторий.

## Управление DonkLift

- Helper, установленный в каждый экземпляр погрузчика, нельзя направлять на
  глобальный `GetPlayerPawn(0)`: после выхода cast теряет покинутый погрузчик и
  не может обнулить его газ, а при двух погрузчиках несколько helpers могут
  писать в один активный pawn. Рабочая граница владения — `GetParentActor()`
  child actor-а плюс `IsPlayerControlled()`.
- Точные `-1`, `0`, `1` одновременно являются цифровыми командами игры.
  Собственное активное состояние нельзя оставлять ровно на этих значениях:
  рабочие sentinel-значения — `0.9999` и `0.0001`.
- Опрос клавиатуры отдельно от штатной записи оси создавал гонки и залипание.
  Проверенный UE4SS-путь — pre-hook native getter после записи игры.
- Сброс `SteeringInput` при выходе прекращает ввод, но не обязан визуально
  выпрямлять колёса: их положение хранится ещё где-то в физике погрузчика.

## Стандартные подсказки

- `BP_VoyageIngameForklift.KeybindRoot` существует, но не является runtime-host
  видимого ряда `E/H`. Видимый host —
  `BP_DynamicPlayerInputHorizontalWidget_Bottom.ContextInputActionsRoot`.
- Добавление Enhanced Input mappings, action events и даже сериализованной
  замены `IMC_Forklift_Keyboard` не заставляет стандартный HUD автоматически
  показать новые действия.
- Самостоятельный viewport overlay доказал, что HUD-child выполняется, но был
  архитектурно неверен: он оставался видимым при pause menu и не наследовал
  lifecycle стандартных подсказок.
- `VoyageVehiclePawn.GetProvidedActionsBP` нельзя объявлять в editor-only
  stand-in класса `VoyageVehicleForkliftPawn`, даже если Blueprint-граф с таким
  именем успешно компилируется. Cooked override ссылается на конкретную parent
  UFunction; нативная registration table доказала, что настоящий owner —
  базовый `VoyageVehiclePawn`. Именно эта ошибка объясняет прежний no-op
  `QuitGame` и требует исправления stand-in hierarchy перед повторным probe.
- Нельзя придумывать отсутствующие enum members для удобства editor-стенда.
  Реальный `EPlayerInputInterfaceActionType` — `Central=0`, `Context=1`,
  `Hidden=2`; старый `Action=0` компилировался локально, но не существовал в
  runtime enum.
- `InteractiveInterface.GetInteractiveProvidedActions` относится к
  взаимодействию персонажа с объектом снаружи. Его наличие в оригинальном
  forklift Blueprint не доказывает связь с панелью подсказок водителя.
- Прямой override `VoyagePlayerInputInterfaces.GetProvidedActions` на старом
  stand-in forklift-child также не вызвался. Не выводить из этого, что dynamic
  HUD обязательно опрашивает другой объект: зеркальная native/interface
  иерархия тогда ещё не соответствовала игре.
- Нельзя проверять producer заменой одного action-поля другим уже используемым
  полем того же объекта. Horn→Exit убрал дубликат из HUD, но привязал к E два
  несовместимых поведения: выход перестал работать, появился horn-звук, затем
  игра упала. Для следующих substitution-пробов нужен уникальный action без
  второй активной привязки на том же погрузчике.
- После двух независимых no-op (`GetProvidedActionsBP` и
  `GetProvidedActions`) нельзя было продолжать менять action-структуру. Этот
  стоп оказался полезен: статический разбор восстановил ошибочного owner-а и
  точный native producer. Следующий override допустим только после воспроизведения
  реальной base-class hierarchy.
- Не приписывать функцию классу только потому, что её строка лежит рядом с
  именами полей этого класса в executable. Генерированные registration records
  разных типов идут подряд; так `SetupPlayerControls`/`TeardownPlayerControls`
  были ошибочно приписаны `VoyageInputControlsComponent`, хотя их owner —
  `VoyageInGameVehicleWidget`. Owner подтверждается таблицей регистрации, а не
  близостью строк.
- `UUserWidget::GetWidgetFromName` не нашёлся как Blueprint-callable UFunction
  в editor-стенде 5.7.4. Для child HUD, чей настоящий parent подставляется
  только при упаковке, рабочий editor-приём — объявить в placeholder-parent
  одноимённое поле нужного типа и обращаться к нему как к наследованной
  Blueprint-переменной. Не добавлять выдуманный native UFUNCTION: в shipping
  классе его не будет.
- UAssetAPI 2.0 с поддержкой UE 5.7 всё равно может оставить экспорт
  game-specific Widget Blueprint как `RawExport`, даже когда CUE4Parse
  восстанавливает его свойства через тот же mapping. Нельзя считать
  `RawExport` отсутствием объекта и нельзя менять байты по одному совпадению.
  Для прямого E/H-проба сначала сравнены Bottom/Center, восстановлены границы
  unversioned header/value/slot, а патчер принимает только полную известную
  17-байтовую сигнатуру Voyage 5.7.4.
- Запись `bRegisterFromWidgetOwner=true` из parent HUD `Construct` дала no-op:
  вложенная dynamic-input панель успевает выбрать ветку регистрации раньше.
  Поздний runtime write нельзя принимать за проверку serialized/default
  значения при создании дочернего виджета.
- Native action path использует `TSet`, а не стабильную сортировку по
  `Priority` или порядку extension array. Перестановка «первого ребёнка в
  конец» дала нужный `E H X C` только для одного наблюдавшегося sparse-set
  порядка. Даже корректная одноразовая перестановка X/C по object identity не
  пережила ESC/resume: игра повторно заполнила ряд без реконструкции owning
  HUD. Постоянный polling ради косметического порядка отклонён. Production-мод
  оставляет порядок стандартных actions на усмотрение игры и вообще не
  подменяет HUD.

## Сборка и поставка IoStore

- При прямом редактировании Blueprint bytecode адреса переходов используют VM
  `iCode`, а не файловый offset и не offset, показываемый CUE4Parse. Вставка
  инструкций требует пересчитать все затронутые jump targets и wrapper-entry
  points по результату сериализации; совпадение отображаемого offset ничего не
  доказывает. Текущий DonkLift-мод этот хрупкий путь больше не использует.
- `retoc` release v0.1.5 предлагал режим UE 5.7, но не содержал нужных import
  type hierarchies для разбора этих Voyage assets через UAssetAPI/UAssetGUI.
  Не доверять одному наличию версии в CLI: проверять реальный round-trip и
  `to-zen` на узком asset до построения инструмента вокруг конкретного релиза.
- Legacy `.uasset`, извлечённый старой версией `retoc`, может иметь тот же
  `.uexp` и правдоподобное содержимое, но быть непригодным для обратного
  `to-zen` после relocation (`failed to fill whole buffer`). Перед сборкой
  заново извлекать оригинал текущим зафиксированным инструментом и считать
  успешный `to-zen`/`verify` обязательной проверкой формата заголовка.
- Извлечение из всего каталога `Paks` при установленном override-моде может
  вернуть заменённый asset вместо оригинала игры. Для обновления snapshot
  сначала убрать дополнительные контейнеры при закрытой игре; инструмент
  извлечения по умолчанию теперь останавливается на такой неоднозначности.
- Подменённый child может выглядеть правдоподобным «оригиналом» и даже иметь
  ожидаемые два вхождения собственного package path. Если перенести его на
  путь parent, уже существующая ссылка child на этот parent превращается в
  самонаследование: контейнер проходит `retoc verify`, а игра падает со
  `STACK_OVERFLOW`. Упаковщик обязан до замены отвергать source asset, который
  уже содержит relocated-parent path; после извлечения также проверять, что
  фильтр реально дал хотя бы один `.uasset`.
- Перемещённый оригинальный forklift Blueprint — snapshot конкретной версии
  игры. Его нельзя коммитить как исходник и нельзя повторно использовать после
  обновления. Перед сборкой его извлекают заново и проверяют fingerprint.
- Подмена package name делается только между заранее проверенными
  равнодлинными путями. Нельзя ослаблять assertion и патчить «похожее» число
  строковых совпадений: изменение длины ломает cooked layout.
- Broad `CookDir` оказался не более удобным вариантом: Unreal проследовал по
  editor dependencies, начал компилировать глобальные shaders и готовить
  посторонние Engine/OpenWorld packages. Production cook — пять узких
  `-CookSinglePackageNoRefs` запусков.
- Отдельные control/HUD containers были полезны как диагностическая граница,
  но HUD-подмена удалена из production после отказа от принудительного порядка.
  Валидированный результат собирается в один `DonkLiftKeyboardControl_P` и
  проверяется без оставшегося probe-пака.
- Успешный `retoc verify` доказывает целостность контейнера, но не runtime
  совместимость Blueprint, input и HUD. После cleanup/consolidation всё равно
  нужна полная проверка в игре.
- Relocation-проб `WBP_InteractIndicator` для build `24990438` прошёл cook,
  `retoc verify` и независимый разбор bytecode/CDO, но упал при старте на
  `FAsyncLoadingThread` с null read до выполнения `PreConstruct`. Чистый
  двухassetный inventory и правдоподобный child CDO не доказывают, что
  reconstructed-native-parent совместим с конкретным Widget Blueprint.
  Считать отклонённой всю эту конкретную архитектуру, а не менять следующий
  граф поверх неё.
- Нельзя перенаправлять package import только потому, что искомый путь
  встречается в `.uasset` один раз. В `BP_VoyageIngameHud` единственный import
  `WBP_InteractIndicator` одновременно обслуживает CDO-свойство
  `IndicatorSubClass` и встроенный экземпляр в widget tree. Equal-length
  замена пути меняет обоих потребителей; такой probe был отклонён статически и
  не устанавливался. Для точечной замены нужен отдельный CDO delta либо
  структурное добавление нового import, а не глобальная подмена существующего.
- Нельзя считать inherited CDO property безопасной только потому, что
  reconstructed native prefix имеет ожидаемый размер и generated Blueprint
  успешно cook-ится. В probe3 для `BP_VoyageIngameHud` значение, заданное как
  `IndicatorSubClass`, при независимом разборе вместе с оригинальным parent
  декодировалось как `ButtonInfoContainer_Action_1`. Это несовпадение
  unversioned-property индекса было найдено до установки. Для такого изменения
  нужен структурный CDO patch с проверкой точного имени свойства, а не ещё один
  mirror/child вариант.
- В UE 5.8 Voyage `FObjectImport.PackageName` сериализуется и для filtered
  cooked packages. UAssetAPI, как и исходный retoc 0.1.5, ошибочно пропускал
  это поле при `IsFilterEditorOnly`, после чего import-map выглядел
  правдоподобно, но был сдвинут. Однако глобальное снятие этого ограничения
  тоже неверно: filtered fixtures UE4 и UE 5.3--5.7 используют старый layout
  без `PackageName`. Чтение и запись должны включать поле только при явно
  выбранном `VER_UE5_8`; unchanged roundtrip обязан быть побайтно идентичным
  как для текущего Voyage, так и для более старых regression fixtures. У
  нового import также `PackageName` должен совпадать с его `ObjectName`.
- Нельзя объявлять существующие падения тестов допустимым baseline, сравнив
  только два наших последовательных коммита. Коммит `48b6096` уже содержал
  слишком широкий filtered-import patch и сам внёс десять binary-roundtrip
  регрессий, поэтому сравнение более поздних изменений с `48b6096` или
  `b5c47b7` их скрывало. Для изменения общей сериализации нужно отдельно
  прогнать тот же набор на настоящем upstream merge-base; текущий контрольный
  результат до и после корректного UE 5.8 gate -- `27/27`.
- Имя `.uasset` package не обязано совпадать с внутренним именем Blueprint
  asset. У marker package `WBP_InteractIndicator_M` внутренний asset оставлен
  `WBP_InteractIndicator`, поэтому generated class называется
  `WBP_InteractIndicator_C`. Конструировать имя класса из имени package suffix
  нельзя; сравнивать с reference import от реально cooked child.
- `AddFunctionGraph` для override `BlueprintNativeEvent` автоматически может
  создать вызов родительской реализации и связать его return со служебным
  result pin. Установка default value без разрыва этой связи ничего не меняет:
  такой первый `GetDescriptionFooter` probe статически декомпилировался обратно
  в вызов parent. Перед константным marker override нужно разорвать exec- и
  value-связи auto-parent узла и проверить итоговый cooked bytecode.
- Нельзя по одному actor-child probe решить, что сломано именно наследование,
  если одновременно добавлен BlueprintNativeEvent override. Footer-probe для
  `BP_Module_Diesel_Container` прошёл загрузку package, но через 22 секунды упал
  на GameThread с записью по адресу `0xe0`. Это не прежний async-load/CDO crash,
  однако опыт смешал подмену класса и `GetDescriptionFooter`. Следующий контроль
  обязан сохранить relocation/child и убрать только override; до него отклонён
  конкретный комбинированный пакет, а не паттерн наследования вообще.
- Такой no-override контроль для `BP_Module_Diesel_Container` повторил через 24
  секунды точный GameThread crash: запись по `0xe0`, тот же stack hash и стек.
  Значит, для этого package отклонена сама stock-path actor-child/SCS подмена,
  независимо от `GetDescriptionFooter`. В cooked child обнаружен второй
  `DefaultSceneRoot_GEN_VARIABLE`, templated от одноимённого компонента
  relocated parent; это ведущий механизм archetype collision. Рабочий child
  другого Actor, например DonkLift, не доказывает безопасность этого package:
  сравнивать нужно также SCS/component template identity и lifecycle.
- Подмена native component import на Blueprint-child может сохранить исходный
  Actor, его CDO, SCS owner и byte-identical `.uexp`, пройти cook и
  `retoc verify`, но всё равно нарушить более широкий runtime-контракт класса.
  Для двух Diesel socket templates такая подмена не уронила игру, зато
  приклеила персонажа к кораблю и визуально оторвала часть кабелей. Поэтому
  `VoyageModuleSocketViewComponent` нельзя считать только producer-ом текста:
  его точная native identity участвует также в attachment/cable/interaction
  поведении.
- Наличие свежего файла с именем `Mappings.usmap` не доказывает наличие схем.
  Ложный automatic `GUObjectArray` resolver для Voyage UE 5.8 создал корректный
  по заголовку 28-байтовый USMAP с нулевым payload, тогда как полный manual dump
  имел размер 2,141,952 байта. Любой parser workflow обязан проверять manifest,
  fingerprint, hash, payload size и несколько обязательных schema names до
  чтения asset. Даже текущий jmap с исправленным UE 5.6+ `MinAlignment`
  повторил ложный automatic result; только structural `.data` scan и явный
  `GUObjectArray` дали полный dump, байт-в-байт совпавший с прежним. Но одного
  структурно допустимого снимка тоже недостаточно: на ещё прогревающемся
  процессе такой кандидат прошёл pointer/count проверки, после чего dumper
  запросил 231 GB памяти и завершился с `0xC0000409`. Тот же адрес и процесс
  после стабилизации дали точный известный mapping. Перед dump нужны как
  минимальный возраст процесса, так и несколько одинаковых снимков адреса,
  pointers и object/chunk counts подряд.
- Windows exit code `0xE0434352` сам по себе означает необработанное managed
  exception, а не поломку CLR. Для наших повторявшихся диалогов журнал
  `.NET Runtime` показал ожидаемые `InvalidDataException`,
  `DirectoryNotFoundException` и LINQ assertion failures из
  `VoyageAssetPatcher`/`VoyageAssetInspector`, а не runtime fault. CLI tools
  должны перехватывать верхнеуровневое exception, печатать краткую ошибку и
  возвращать exit code `1`, чтобы диагностический отказ не выглядел как crash
  самого `.NET`.
- Два вызова `retoc to-zen` на идентичном staging tree могут записать chunks в
  разном физическом порядке. Тогда SHA-256 целых `.ucas/.utoc` различаются,
  хотя chunk ID, asset path, размер и content hash полностью совпадают. Для
  сравнения независимых сборок использовать отсортированный semantic inventory;
  целые файловые хеши остаются обязательными только между конкретным
  подготовленным пакетом и его установленной копией.
- Успешный `UAsset.VerifyBinaryEquality()` не доказывает, что UAssetGUI разобрал
  каждый export. `RawExport` сохраняет неизвестные байты и поэтому может дать
  побайтно идентичную unchanged-сериализацию одновременно с диалогом
  `Failed to parse N exports`. Обратная комбинация тоже возможна: asset может
  не иметь raw fallback и всё же изменить байты при записи. Массовая проверка
  открытия обязана независимо требовать ноль `RawExport` и положительное
  binary equality; исключения и неприменённые mappings являются отдельными
  отказами.
- Командный `UAssetGUI fromjson` не принимает engine version и потому не
  эквивалентен обычному GUI Save из asset, открытого с явно выбранным UE 5.8.
  Для Voyage он может пересобрать JSON в смешанном layout: один слой следует
  данным JSON/старой версии, а filtered-import header — текущему условию API.
  До исправления интерфейса и regression test проверять изменённую пару надо
  прямым открытием с `VER_UE5_8`, `VerifyBinaryEquality`, повторной записью в
  том же режиме и независимым CUE4Parse/`retoc` проходом.
- При динамическом построении schema из Blueprint нельзя загружать только
  непосредственный dependency asset. Индексы unversioned properties включают
  всю цепочку родителей. Кроме того, нельзя навсегда оставлять путь в
  `PathsAlreadyProcessedForSchemas`, если preload завершился с отсутствующими
  зависимостями: после извлечения нового transitive parent следующий проход
  обязан повторить этот asset.
- Native custom serialization не становится обычной отражённой структурой от
  наличия свежего USMAP. `InstancedPropertyBag` в
  `ABP_HologramPedestal` начинался собственным четырёхбайтовым `bHasData`; попытка
  разобрать его как mapped `InstancedPropertyBag.Value` сдвинула поток до
  ложного `FPackageIndex`. Поддерживать только доказанный вариант формата и
  явно отвергать неизвестный, а не читать соседние bytes как свойства.
- Отсутствующий в mappings импортированный native enum не требует выдумывать
  его members. Для unchanged roundtrip допустим явно помеченный numeric
  fallback (`UASSETAPI_INVALID_ENUM_IDX_<n>`), если ширина underlying property
  известна, байты сохраняются, а stress report остаётся `notice`. В текущем
  Voyage таким случаем является `/Script/FunctionalTesting.EComparisonMethod`.
- `RigHierarchy` и `RigVM` имеют собственные native serializers. Их нельзя
  считать обычными `NormalExport` и нельзя объявлять полностью разобранными.
  Точное opaque-сохранение может пройти unchanged binary equality, но должно
  оставаться отдельным `notice` и не разрешает структурное редактирование.

## UE4SS HUD

- `pcall` не защищает от падения внутри native marshaling.
- Lua-таблица вида `{ KeyName = "W" }` не является настоящим `FKey` для
  `IsInputKeyDown` и уже вызывала native crash.
- Создание нового `FText` требовало version-pinned сигнатуры конструктора.
  Даже уникальный pattern нужно перепроверять после каждого изменения exe.
- `KismetInternationalizationLibrary.GetCurrentLanguage` не отражал язык меню
  Voyage. Проверенным источником был
  `VoyageGameUserSettings.CustomSettings.LanguageType`.
