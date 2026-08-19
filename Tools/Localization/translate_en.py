"""Fill the English localisation with real translations.

Run:
    python3 Tools/Localization/translate_en.py

WHY THIS EXISTS
---------------
`Content/Localization/Game/en/Game.po` had all 260 `msgstr` entries empty.
Unreal falls back to the source string when a translation is blank, and the
source culture here is Russian (`NativeCulture=ru` in
`Config/Localization/Game.ini`). So selecting English still produced Russian
UI text mixed with the engine's own English labels - the "кириллица и латиница
вперемешку" the user reported.

This script is data, not cleverness: an explicit ru -> en mapping applied to
the existing .po. Keeping it in the repo means the translation is
reproducible and reviewable rather than a one-off hand edit of a generated
file.

AFTER RUNNING
-------------
`Game.po` is only the editable source. The engine loads the compiled
`Game.locres`. Recompile with:

    UnrealEditor-Cmd RedAlert4.uproject \
        -run=GatherText -config=Config/Localization/Game.ini \
        -EnableSCC=false -DisableSCCSubmit

or the narrower step, which only regenerates the .locres from the .po:

    UnrealEditor-Cmd RedAlert4.uproject \
        -run=InternationalizationExport \
        -config=Config/Localization/Game.ini -ImportLoc

NOTE ON \\n
-----------
Strings in the .po carry literal backslash-n two-character sequences, not real
newlines. The mapping below is written with the same literal `\\n`, and
matching is done on the raw .po text, so multi-line panels survive intact.
"""

import re
import sys
from pathlib import Path

PO_PATH = Path("Content/Localization/Game/en/Game.po")

# Russian source -> English translation.
# Uppercase is preserved because the UI uses caps as a deliberate style.
TRANSLATIONS = {
    # --- navigation / shell -------------------------------------------------
    "ГЛАВНАЯ": "HOME",
    "КАМПАНИЯ": "CAMPAIGN",
    "СЕТЕВАЯ ИГРА": "MULTIPLAYER",
    "ИСПЫТАНИЯ": "CHALLENGES",
    "КАЗАРМА": "BARRACKS",
    "НАСТРОЙКИ": "SETTINGS",
    "СХВАТКА": "SKIRMISH",
    "РЕДАКТОР": "EDITOR",
    "ЭНЦИКЛОПЕДИЯ": "ENCYCLOPEDIA",
    "МОДИФИКАЦИИ": "MODS",
    "ВЫХОД": "EXIT",
    "ОБУЧЕНИЕ": "TUTORIAL",
    "<  НАЗАД": "<  BACK",
    "‹  НАЗАД": "‹  BACK",
    "ОТМЕНА": "CANCEL",
    "ВЫЙТИ": "QUIT",
    "ПОВТОРИТЬ": "RETRY",
    "ВЫЙТИ В МЕНЮ": "QUIT TO MENU",
    "ПАУЗА": "PAUSED",
    "ПАУЗА // ТАКТИЧЕСКОЕ МЕНЮ": "PAUSE // TACTICAL MENU",
    "СИМУЛЯЦИЯ ПРИОСТАНОВЛЕНА": "SIMULATION SUSPENDED",
    "► ПРОДОЛЖИТЬ ИГРУ": "► RESUME GAME",
    "↻ ПЕРЕЗАПУСТИТЬ МАТЧ": "↻ RESTART MATCH",
    "⚙ НАСТРОЙКИ": "⚙ SETTINGS",
    "⎋ ВЫЙТИ В ГЛАВНОЕ МЕНЮ": "⎋ QUIT TO MAIN MENU",
    "✕ ВЫЙТИ НА РАБОЧИЙ СТОЛ": "✕ QUIT TO DESKTOP",
    "ПРЯМОЕ УПРАВЛЕНИЕ": "DIRECT CONTROL",
    "[F] ВЫЙТИ ИЗ РЕЖИМА УПРАВЛЕНИЯ": "[F] EXIT DIRECT CONTROL",

    # --- titles / headings --------------------------------------------------
    "ТОВАРИЩ КОМАНДИР": "COMRADE COMMANDER",
    "ВЫБОР КАМПАНИИ": "CAMPAIGN SELECT",
    "О ВЫБРАННОЙ КАМПАНИИ": "SELECTED CAMPAIGN",
    "ПРОГРЕСС КАМПАНИИ": "CAMPAIGN PROGRESS",
    "ПРОДОЛЖИТЬ КАМПАНИЮ": "CONTINUE CAMPAIGN",
    "НАЧАТЬ КАМПАНИЮ": "START CAMPAIGN",
    "СОВЕТСКОЕ ВЕРХОВНОЕ КОМАНДОВАНИЕ": "SOVIET SUPREME COMMAND",
    "КОМАНДУЮЩИЙ": "COMMANDER",
    "СВОДКА НОВОСТЕЙ": "NEWS FEED",
    "ЗАВЕРШИТЬ СЕАНС?": "END SESSION?",
    "ПОБЕДА": "VICTORY",
    "ПОРАЖЕНИЕ": "DEFEAT",
    "ГЛАВНОЕ КОМАНДОВАНИЕ": "HIGH COMMAND",
    "ВЫБОР ФРАКЦИИ": "FACTION SELECT",
    "СЕТЕВОЕ ЛОББИ": "MULTIPLAYER LOBBY",
    "НАСТРОЙКИ СИСТЕМЫ": "SYSTEM SETTINGS",
    "ОПЕРАТИВНЫЕ ДАННЫЕ": "OPERATIONAL DATA",
    "РАДАР": "RADAR",
    "СПРАВКА ОБ ОБЪЕКТЕ": "OBJECT INFO",
    "ОБЪЕКТ НЕ ВЫБРАН": "NO OBJECT SELECTED",
    "ПРОИЗВОДСТВО": "PRODUCTION",
    "ОЧЕРЕДЬ ПУСТА": "QUEUE EMPTY",
    "СИСТЕМНОЕ СООБЩЕНИЕ": "SYSTEM MESSAGE",
    "ТЕХНОЛОГИЧЕСКОЕ ДЕРЕВО": "TECH TREE",
    "ЭНЦИКЛОПЕДИЯ ВОЙНЫ": "WAR ENCYCLOPEDIA",
    "КАРТА ОПЕРАЦИЙ: ЕВРОПА": "OPERATIONS MAP: EUROPE",
    "ЗАЩИЩЁННЫЙ ВИДЕОКАНАЛ": "SECURE VIDEO CHANNEL",
    "ПОДГОТОВКА ОПЕРАЦИИ": "OPERATION SETUP",

    # --- factions -----------------------------------------------------------
    "СССР": "USSR",
    "АЛЬЯНС": "ALLIANCE",
    "ВОСТОЧНАЯ КОАЛИЦИЯ": "EASTERN COALITION",
    "ВОСТОЧНАЯ\\nКОАЛИЦИЯ": "EASTERN\\nCOALITION",
    "ХРОНОЛЕГИОН": "CHRONOLEGION",

    # --- faction mottos -----------------------------------------------------
    "СЛАВА РОДИНЕ. БУДУЩЕЕ ЗА НАМИ.":
        "GLORY TO THE MOTHERLAND. THE FUTURE IS OURS.",
    "СВОБОДА. ТОЧНОСТЬ. ПРЕВОСХОДСТВО.":
        "FREEDOM. PRECISION. SUPERIORITY.",
    "ЕДИНСТВО СОЗДАЁТ ПОБЕДУ.": "UNITY FORGES VICTORY.",
    "ВРЕМЯ — НАШЕ ОРУЖИЕ.": "TIME IS OUR WEAPON.",

    # --- faction descriptions ----------------------------------------------
    "Возглавьте возрождённый Советский Союз в борьбе за мировое господство. "
    "Тяжёлая броня, дисциплина и несокрушимая воля сокрушат врагов революции.":
        "Lead the reborn Soviet Union in the struggle for world dominance. "
        "Heavy armour, discipline and unbreakable will shall crush the "
        "enemies of the revolution.",
    "Соберите коалицию демократических держав. Используйте авиацию, "
    "высокоточное оружие и мобильные силы для защиты свободного мира.":
        "Assemble a coalition of democratic powers. Use air power, precision "
        "weapons and mobile forces to defend the free world.",
    "Объедините промышленную мощь Востока. Развивайте производство, боевые "
    "механизмы и контроль энергетических узлов.":
        "Unite the industrial might of the East. Expand production, war "
        "machines and control of power nodes.",
    "Командуйте армией вне времени. Искажайте поле боя, перемещайте войска "
    "через хронокоридоры и переписывайте исход войны.":
        "Command an army outside of time. Warp the battlefield, move troops "
        "through chrono-corridors and rewrite the outcome of the war.",

    # --- resources / HUD ----------------------------------------------------
    "КРЕДИТЫ": "CREDITS",
    "ЭНЕРГИЯ": "POWER",
    "ВОЙСКА": "UNITS",
    "ВРЕМЯ": "TIME",
    "ЭНЕРГИЯ  {0} / {1}": "POWER  {0} / {1}",
    "ЗДОРОВЬЕ: {0}%": "HEALTH: {0}%",
    "Выбрано объектов: {0}": "Selected: {0}",
    "Вражеский объект": "Enemy object",
    "Союзный объект": "Allied object",
    "Кликните по юниту или зданию": "Click a unit or building",
    "{0} — ВЫБЕРИТЕ МЕСТО": "{0} — CHOOSE LOCATION",
    "{0} — ПАУЗА": "{0} — PAUSED",

    # --- build blockers -----------------------------------------------------
    "нет средств": "insufficient funds",
    "нет здания": "no building",
    "нет завода": "no factory",
    "очередь полна": "queue full",
    "матч окончен": "match over",
    "Недостаточно энергии": "Insufficient power",
    "Недостаточно средств": "Insufficient funds",

    # --- structures ---------------------------------------------------------
    "Штаб-квартира": "Headquarters",
    "Электростанция": "Power Plant",
    "Переработчик руды": "Ore Refinery",
    "Казармы": "Barracks",
    "Завод техники": "War Factory",
    "Радарный комплекс": "Radar Complex",
    "Штаб-квартира СССР": "USSR Headquarters",
    "Казармы СССР": "USSR Barracks",
    "Завод техники СССР": "USSR War Factory",
    "Штаб-квартира Альянса": "Alliance Headquarters",
    "Электростанция Альянса": "Alliance Power Plant",
    "Казармы Альянса": "Alliance Barracks",
    "Завод техники Альянса": "Alliance War Factory",
    "Пулеметная турель": "Machine Gun Turret",
    "ПВО турель": "AA Turret",

    # --- units --------------------------------------------------------------
    "Стрелок «Рубеж»": "Rubezh Rifleman",
    "ПВО команда «Заслон»": "Zaslon AA Team",
    "Мастер-инженер": "Master Engineer",
    "Разведчик «Рысь»": "Lynx Scout",
    "Основной танк «Гранит»": "Granit Main Battle Tank",
    "РСЗО «Зарево»": "Zarevo Rocket Artillery",
    "Пехотинец «Страж»": "Guardian Infantry",
    "Ракетчик «Лансер»": "Lancer Missile Trooper",
    "Полевой инженер": "Field Engineer",
    "Разведчик «Пустельга»": "Kestrel Scout",
    "Танк «Оплот»": "Bulwark Tank",
    "Артиллерия «Оракул»": "Oracle Artillery",
    "Тесла-танк": "Tesla Tank",
    "Призывник": "Conscript",

    # --- EVA / alerts -------------------------------------------------------
    "База атакована": "Base under attack",
    "Наши войска атакованы": "Our forces are under attack",
    "Здание потеряно": "Building lost",
    "Юнит потерян": "Unit lost",
    "Строительство завершено": "Construction complete",
    "Юнит готов": "Unit ready",
    "Месторождение исчерпано": "Ore field depleted",
    "Ожидание приказа командующего.": "Awaiting commander's orders.",

    # --- characters ---------------------------------------------------------
    "АДМИРАЛ ВАРД": "ADMIRAL WARD",
    "ГЕНЕРАЛ ГАО": "GENERAL GAO",
    "ХРОНОС-07": "CHRONOS-07",

    # --- settings tabs ------------------------------------------------------
    "ИЗОБРАЖЕНИЕ": "VIDEO",
    "ЗВУК": "AUDIO",
    "УПРАВЛЕНИЕ": "CONTROLS",
    "ИГРА": "GAMEPLAY",
    "ЯРКОСТЬ  54%": "BRIGHTNESS  54%",
    "МАСШТАБ ИНТЕРФЕЙСА  100%": "UI SCALE  100%",
    "МУЗЫКА  80%": "MUSIC  80%",
    "ЭФФЕКТЫ  90%": "EFFECTS  90%",

    # --- status lines -------------------------------------------------------
    "УРОВЕНЬ 47  //  СЕТЬ ПОДКЛЮЧЕНА": "LEVEL 47  //  NETWORK ONLINE",
    "1927  —  2047  //  АРХИВ КОМАНДОВАНИЯ":
        "1927  —  2047  //  COMMAND ARCHIVE",
    "УРОВЕНЬ 27": "LEVEL 27",
    "28 750 / 34 000 ОП": "28,750 / 34,000 XP",
    "v1.0.0  //  RU": "v1.0.0  //  EN",
    "ЗАЩИЩЁННАЯ СЕТЬ КОМАНДОВАНИЯ  //  КАНАЛ 04":
        "SECURE COMMAND NETWORK  //  CHANNEL 04",
    "СВЯЗЬ: СТАБИЛЬНАЯ\\nШИФРОВАНИЕ: АКТИВНО":
        "LINK: STABLE\\nENCRYPTION: ACTIVE",
    "ДОПУСК: АЛЬФА\\nСЕАНС: ЗАШИФРОВАН":
        "CLEARANCE: ALPHA\\nSESSION: ENCRYPTED",
    "СИСТЕМА ГОТОВА  ·  НАВИГАЦИЯ АКТИВНА  ·  РУССКИЙ":
        "SYSTEM READY  ·  NAVIGATION ACTIVE  ·  ENGLISH",
    "RA4 // BUILD 1.0.0": "RA4 // BUILD 1.0.0",
    "СССР // ОПЕРАЦИЯ": "USSR // OPERATION",
    "КРЕДИТЫ  12 450": "CREDITS  12,450",
    "ЭНЕРГИЯ  780 / 920": "POWER  780 / 920",

    # --- news cards ---------------------------------------------------------
    "НОВАЯ ФРАКЦИЯ: АВАНГАРД": "NEW FACTION: VANGUARD",
    "Технологическое превосходство.\\nТактическое устрашение.":
        "Technological superiority.\\nTactical intimidation.",
    "ОБНОВЛЕНИЕ БАЛАНСА 1.2": "BALANCE UPDATE 1.2",
    "Корректировка юнитов,\\nулучшения и исправления.":
        "Unit adjustments,\\nimprovements and fixes.",
    "СЕЗОННЫЙ ПРОПУСК": "SEASON PASS",
    "Эксклюзивные награды\\nи ранний доступ к контенту.":
        "Exclusive rewards\\nand early content access.",

    # --- profile ------------------------------------------------------------
    "РАНГ\\nРЕПУТАЦИЯ\\nПОБЕДЫ\\nПОРАЖЕНИЯ":
        "RANK\\nREPUTATION\\nWINS\\nLOSSES",
    "ГЕНЕРАЛ-МАЙОР\\n12 450\\n87\\n19":
        "MAJOR GENERAL\\n12,450\\n87\\n19",

    # --- dialogs ------------------------------------------------------------
    "Соединение с командным центром будет разорвано.\\nНесохранённые данные "
    "текущей операции будут потеряны.":
        "The link to command will be severed.\\nUnsaved data from the current "
        "operation will be lost.",
    "Операция завершена успешно. Можно сразу начать матч заново или безопасно "
    "выйти.":
        "Operation completed successfully. You may restart the match "
        "immediately or exit safely.",
    "База потеряна. Попробуйте еще раз или завершите текущую демо-сессию.":
        "Base lost. Try again or end the current demo session.",
    "Операция приостановлена. Продолжите бой, откройте настройки, сохраните "
    "прогресс или вернитесь в командный центр.":
        "Operation paused. Resume the battle, open settings, save progress or "
        "return to the command centre.",

    # --- campaign / mission text -------------------------------------------
    "Выберите направление операции. Кампания, сетевой бой и системные "
    "параметры доступны из защищённого командного центра.":
        "Choose your line of operations. Campaign, multiplayer and system "
        "settings are available from the secure command centre.",
    "КОМАНДУЮЩИЙ\\nГотов к операции\\n\\nСОСТОЯНИЕ СЕТИ\\nСтабильное\\n\\n"
    "ТЕАТР ВОЙНЫ\\nЕвропа // 2049":
        "COMMANDER\\nReady for operations\\n\\nNETWORK STATUS\\nStable\\n\\n"
        "THEATRE OF WAR\\nEurope // 2049",
    "СССР — красно-чёрная доктрина подавления. Альянс — сине-стальная "
    "мобильность. Восточная коалиция — нефрит и золото. Хронолегион — "
    "технологии вне времени.":
        "USSR — a red-and-black doctrine of suppression. Alliance — "
        "blue-steel mobility. Eastern Coalition — jade and gold. "
        "Chronolegion — technology beyond time.",
    "ОПЕРАЦИЯ 01\\nПепел столицы\\n\\nСЛОЖНОСТЬ\\nВетеран\\n\\n"
    "ВЫБРАНА ФРАКЦИЯ\\nСССР":
        "OPERATION 01\\nAshes of the Capital\\n\\nDIFFICULTY\\nVeteran\\n\\n"
        "SELECTED FACTION\\nUSSR",
    "ОПЕРАЦИЯ: ПЕПЕЛ СТОЛИЦЫ": "OPERATION: ASHES OF THE CAPITAL",
    "ОПЕРАЦИЯ: СЕВЕРНЫЙ ЩИТ": "OPERATION: NORTHERN SHIELD",
    "ОПЕРАЦИЯ: ЗОЛОТОЙ РАССВЕТ": "OPERATION: GOLDEN DAWN",
    "ОПЕРАЦИЯ: ПАРАДОКС": "OPERATION: PARADOX",
    "КАМПАНИЯ АЛЬЯНСА": "ALLIANCE CAMPAIGN",
    "КАМПАНИЯ ВОСТОЧНОЙ КОАЛИЦИИ": "EASTERN COALITION CAMPAIGN",
    "КАМПАНИЯ ХРОНОЛЕГИОНА": "CHRONOLEGION CAMPAIGN",
    "БРИФИНГ: ПЕПЕЛ СТОЛИЦЫ": "BRIEFING: ASHES OF THE CAPITAL",
    "ВОСТОЧНАЯ КОАЛИЦИЯ: ГЕНЕРАЛ ГАО": "EASTERN COALITION: GENERAL GAO",
    "ТРЕВОГА: БРОНЕКОЛОННА": "ALERT: ARMOURED COLUMN",
    "ПРЕДУПРЕЖДЕНИЕ: АВИАНАЛЁТ": "WARNING: AIR RAID",
    "МОРСКАЯ ОПЕРАЦИЯ: ЛЕДЯНОЙ ПРОЛИВ": "NAVAL OPERATION: ICE STRAIT",
    "ВОЗДУШНАЯ ОПЕРАЦИЯ: ЧИСТОЕ НЕБО": "AIR OPERATION: CLEAR SKY",
    "ХРОНО-ОРУЖИЕ: ГОТОВНОСТЬ 94%": "CHRONO WEAPON: 94% READY",

    # --- campaign progress panels ------------------------------------------
    "МИССИИ ПРОЙДЕНО          06 / 18\\nДОП. ЗАДАНИЯ             09 / 36\\n"
    "СЛОЖНОСТЬ                ВЕТЕРАН":
        "MISSIONS COMPLETE        06 / 18\\nSIDE OBJECTIVES          09 / 36\\n"
        "DIFFICULTY               VETERAN",
    "МИССИИ ПРОЙДЕНО          02 / 16\\nДОП. ЗАДАНИЯ             03 / 28\\n"
    "СЛОЖНОСТЬ                ОФИЦЕР":
        "MISSIONS COMPLETE        02 / 16\\nSIDE OBJECTIVES          03 / 28\\n"
        "DIFFICULTY               OFFICER",
    "МИССИИ ПРОЙДЕНО          00 / 15\\nДОП. ЗАДАНИЯ             00 / 30\\n"
    "СЛОЖНОСТЬ                НЕ ВЫБРАНА":
        "MISSIONS COMPLETE        00 / 15\\nSIDE OBJECTIVES          00 / 30\\n"
        "DIFFICULTY               NOT SET",
    "МИССИИ ПРОЙДЕНО          00 / 12\\nВРЕМЕННЫЕ УЗЛЫ           00 / 24\\n"
    "СЛОЖНОСТЬ                НЕ ВЫБРАНА":
        "MISSIONS COMPLETE        00 / 12\\nTEMPORAL NODES           00 / 24\\n"
        "DIFFICULTY               NOT SET",

    # --- HUD production / recon panels -------------------------------------
    "ТЕСЛА-ТАНК\\nТяжёлая броня  ·  очередь 01":
        "TESLA TANK\\nHeavy armour  ·  queue 01",
    "ПРИЗЫВНИК ×5\\nКазармы  ·  очередь 02\\n\\nМИГ «КУЗНЕЦ»\\n"
    "Аэродром  ·  ожидание":
        "CONSCRIPT ×5\\nBarracks  ·  queue 02\\n\\nMIG \\\"KUZNETS\\\"\\n"
        "Airfield  ·  waiting",
    "РАЗВЕДКА // ЮЖНЫЙ МОСТ\\nКОНТАКТЫ: 12  ·  УГРОЗА: ВЫСОКАЯ":
        "RECON // SOUTH BRIDGE\\nCONTACTS: 12  ·  THREAT: HIGH",
    "ОТРЯД: 3 × ТЕСЛА-ТАНК   ·   12 × ПРИЗЫВНИК   ·   "
    "ПРИКАЗ: УДЕРЖИВАТЬ ПОЗИЦИЮ   ·   СИЛЫ СПЕЦНАЗНАЧЕНИЯ: ГОТОВЫ":
        "SQUAD: 3 × TESLA TANK   ·   12 × CONSCRIPT   ·   "
        "ORDER: HOLD POSITION   ·   SPECIAL FORCES: READY",

    # --- faction battle HUDs -----------------------------------------------
    "БОЕВОЙ HUD СССР\\n\\nКРЕДИТЫ 12 450     ЭНЕРГИЯ 780 / 920\\n\\n"
    "ОЧЕРЕДЬ ПРОИЗВОДСТВА\\nТесла-танк — 72%\\nПризывник ×5\\n\\n"
    "ЗАДАЧА: удержать плацдарм и уничтожить командный узел противника.":
        "USSR COMBAT HUD\\n\\nCREDITS 12,450     POWER 780 / 920\\n\\n"
        "PRODUCTION QUEUE\\nTesla Tank — 72%\\nConscript ×5\\n\\n"
        "OBJECTIVE: hold the beachhead and destroy the enemy command node.",
    "МИНИ-КАРТА\\nСектор: М-14\\n\\nВЫДЕЛЕНО\\n3 × Тесла-танк\\n"
    "12 × Призывник\\n\\nТРЕВОГА\\nНизкая":
        "MINIMAP\\nSector: M-14\\n\\nSELECTED\\n3 × Tesla Tank\\n"
        "12 × Conscript\\n\\nALERT\\nLow",
    "БОЕВОЙ HUD АЛЬЯНСА\\n\\nКРЕДИТЫ 10 280     ЭНЕРГИЯ 640 / 760\\n\\n"
    "ОЧЕРЕДЬ ПРОИЗВОДСТВА\\nСтраж — 54%\\nАвиакрыло «Копьё» ×2\\n\\n"
    "ЗАДАЧА: удержать гавань и защитить конвой.":
        "ALLIANCE COMBAT HUD\\n\\nCREDITS 10,280     POWER 640 / 760\\n\\n"
        "PRODUCTION QUEUE\\nGuardian — 54%\\nSpear Air Wing ×2\\n\\n"
        "OBJECTIVE: hold the harbour and protect the convoy.",
    "МИНИ-КАРТА\\nСектор: N-07\\n\\nВЫДЕЛЕНО\\n2 × Страж\\n4 × Ракетчик\\n\\n"
    "ФЛОТ\\nГотов":
        "MINIMAP\\nSector: N-07\\n\\nSELECTED\\n2 × Guardian\\n"
        "4 × Missile Trooper\\n\\nFLEET\\nReady",
    "БОЕВОЙ HUD ВОСТОЧНОЙ КОАЛИЦИИ\\n\\nКРЕДИТЫ 15 600     "
    "ЭНЕРГИЯ 850 / 940\\n\\nОЧЕРЕДЬ ПРОИЗВОДСТВА\\nШтурмовой мех — 43%\\n"
    "Дрон «Хуан» ×3\\n\\nЗАДАЧА: удержать энергетические узлы.":
        "EASTERN COALITION COMBAT HUD\\n\\nCREDITS 15,600     "
        "POWER 850 / 940\\n\\nPRODUCTION QUEUE\\nAssault Mech — 43%\\n"
        "Huang Drone ×3\\n\\nOBJECTIVE: hold the power nodes.",
    "МИНИ-КАРТА\\nСектор: P-21\\n\\nВЫДЕЛЕНО\\n1 × Штурмовой мех\\n"
    "8 × Пехота\\n\\nРЕЗЕРВ\\nГотов":
        "MINIMAP\\nSector: P-21\\n\\nSELECTED\\n1 × Assault Mech\\n"
        "8 × Infantry\\n\\nRESERVE\\nReady",
    "БОЕВОЙ HUD ХРОНОЛЕГИОНА\\n\\nКРЕДИТЫ 13 040     "
    "ХРОНОЭНЕРГИЯ 72 / 100\\n\\nОЧЕРЕДЬ ПРОИЗВОДСТВА\\n"
    "Темпоральный страж — 68%\\nРазведчик «Эхо» ×2\\n\\n"
    "ЗАДАЧА: стабилизировать временной якорь.":
        "CHRONOLEGION COMBAT HUD\\n\\nCREDITS 13,040     "
        "CHRONO ENERGY 72 / 100\\n\\nPRODUCTION QUEUE\\n"
        "Temporal Guardian — 68%\\nEcho Scout ×2\\n\\n"
        "OBJECTIVE: stabilise the temporal anchor.",
    "ХРОНО-КАРТА\\nСлой: 3-А\\n\\nВЫДЕЛЕНО\\n2 × Страж\\n1 × Якорь\\n\\n"
    "РАЗРЫВ\\nСдержан":
        "CHRONO MAP\\nLayer: 3-A\\n\\nSELECTED\\n2 × Guardian\\n1 × Anchor\\n\\n"
        "RIFT\\nContained",

    # --- lobby --------------------------------------------------------------
    "КАРТА: КРАСНЫЙ ПЕРЕВАЛ\\nРЕЖИМ: СТАНДАРТНЫЙ БОЙ\\n\\n"
    "КОМАНДУЮЩИЙ — СССР — ГОТОВ\\nАДМИРАЛ ВАРД — АЛЬЯНС — ГОТОВ\\n"
    "ГЕНЕРАЛ ГАО — ВОСТОЧНАЯ КОАЛИЦИЯ — ОЖИДАНИЕ\\n"
    "ХРОНОС-07 — ХРОНОЛЕГИОН — ОЖИДАНИЕ":
        "MAP: RED PASS\\nMODE: STANDARD BATTLE\\n\\n"
        "COMMANDER — USSR — READY\\nADMIRAL WARD — ALLIANCE — READY\\n"
        "GENERAL GAO — EASTERN COALITION — WAITING\\n"
        "CHRONOS-07 — CHRONOLEGION — WAITING",
    "ЧАТ КОМАНДОВАНИЯ\\n[20:49] Вард: Готов.\\n"
    "[20:49] Гао: Проверяю связь.\\n\\nПИНГ\\n34 мс":
        "COMMAND CHAT\\n[20:49] Ward: Ready.\\n"
        "[20:49] Gao: Checking link.\\n\\nPING\\n34 ms",

    # --- settings panels ----------------------------------------------------
    "ИЗОБРАЖЕНИЕ\\nРазрешение: 1920×1080\\n"
    "Масштаб интерфейса: автоматически\\n\\nЗВУК\\nМузыка: 80%\\n"
    "Эффекты: 90%\\n\\nУПРАВЛЕНИЕ\\nEnhanced Input: активно":
        "VIDEO\\nResolution: 1920×1080\\nUI scale: automatic\\n\\n"
        "AUDIO\\nMusic: 80%\\nEffects: 90%\\n\\n"
        "CONTROLS\\nEnhanced Input: active",
    "ПРОФИЛЬ\\nКомандующий\\n\\nЛОКАЛИЗАЦИЯ\\nРусский\\n\\nВЕРСИЯ\\n0.1.0":
        "PROFILE\\nCommander\\n\\nLOCALISATION\\nEnglish\\n\\nVERSION\\n0.1.0",

    # --- boot / splash ------------------------------------------------------
    "СИСТЕМА КОМАНДОВАНИЯ И СВЯЗИ\\n\\n"
    "Нажмите любую клавишу, чтобы войти в защищённую сеть.":
        "COMMAND AND COMMUNICATIONS SYSTEM\\n\\n"
        "Press any key to enter the secure network.",
    "СТАТУС СИСТЕМ\\nВсе контуры готовы\\n\\nСЕАНС\\nЗашифрован":
        "SYSTEM STATUS\\nAll circuits ready\\n\\nSESSION\\nEncrypted",

    # --- faction select -----------------------------------------------------
    "СССР — тяжёлая броня и подавление.\\n"
    "АЛЬЯНС — мобильность и точные удары.\\n"
    "ВОСТОЧНАЯ КОАЛИЦИЯ — массовое производство.\\n"
    "ХРОНОЛЕГИОН — технологии вне времени.":
        "USSR — heavy armour and suppression.\\n"
        "ALLIANCE — mobility and precision strikes.\\n"
        "EASTERN COALITION — mass production.\\n"
        "CHRONOLEGION — technology beyond time.",
    "ВЫБРАНО\\nСССР\\n\\nДОКТРИНА\\nШтурм\\n\\nТЕХНОЛОГИИ\\nУровень 1":
        "SELECTED\\nUSSR\\n\\nDOCTRINE\\nAssault\\n\\nTECHNOLOGY\\nTier 1",

    # --- campaign briefings -------------------------------------------------
    "Операция «Северный щит». Сдержите наступление СССР, верните контроль над "
    "портами и откройте маршрут для флота.":
        "Operation Northern Shield. Halt the Soviet advance, retake the ports "
        "and open a route for the fleet.",
    "КОМАНДУЮЩИЙ\\nАдмирал Вард\\n\\nТЕАТР\\nСеверная Атлантика\\n\\n"
    "МИССИЯ\\n01 / 09":
        "COMMANDER\\nAdmiral Ward\\n\\nTHEATRE\\nNorth Atlantic\\n\\n"
        "MISSION\\n01 / 09",
    "Операция «Золотой рассвет». Захватите энергетические узлы и превратите "
    "вражеский плацдарм в опорную базу коалиции.":
        "Operation Golden Dawn. Seize the power nodes and turn the enemy "
        "beachhead into a coalition stronghold.",
    "КОМАНДУЮЩИЙ\\nГенерал Гао\\n\\nТЕАТР\\nТихоокеанский пояс\\n\\n"
    "МИССИЯ\\n01 / 08":
        "COMMANDER\\nGeneral Gao\\n\\nTHEATRE\\nPacific Belt\\n\\n"
        "MISSION\\n01 / 08",
    "Операция «Парадокс». Восстановите временной коридор до того, как "
    "противник закрепится в ключевых точках хронолинии.":
        "Operation Paradox. Restore the temporal corridor before the enemy "
        "entrenches at the key points of the chronoline.",
    "КООРДИНАТОР\\nХронос-07\\n\\nВРЕМЕННОЙ СЛОЙ\\nНестабилен\\n\\n"
    "МИССИЯ\\n01 / 07":
        "COORDINATOR\\nChronos-07\\n\\nTEMPORAL LAYER\\nUnstable\\n\\n"
        "MISSION\\n01 / 07",
    "МИССИЯ 01 — ПЕПЕЛ СТОЛИЦЫ\\nУдержите мост через Рейн.\\n\\n"
    "МИССИЯ 02 — ЖЕЛЕЗНЫЙ КОРИДОР\\nПерережьте линию снабжения Альянса.\\n\\n"
    "МИССИЯ 03 — ПОЛЯРНАЯ НОЧЬ\\n"
    "Захватите станцию раннего предупреждения.":
        "MISSION 01 — ASHES OF THE CAPITAL\\nHold the bridge over the "
        "Rhine.\\n\\nMISSION 02 — IRON CORRIDOR\\nSever the Alliance supply "
        "line.\\n\\nMISSION 03 — POLAR NIGHT\\n"
        "Capture the early warning station.",
    "ВЫБРАННАЯ МИССИЯ\\nПепел столицы\\n\\nСЛОЖНОСТЬ\\nВетеран\\n\\n"
    "НАГРАДА\\nТесла-танк":
        "SELECTED MISSION\\nAshes of the Capital\\n\\nDIFFICULTY\\nVeteran\\n\\n"
        "REWARD\\nTesla Tank",
    "Войска Альянса удерживают городской командный узел. Разверните базу на "
    "восточном берегу, обеспечьте энергоснабжение и уничтожьте узел до "
    "прибытия подкрепления.":
        "Alliance forces hold the urban command node. Deploy a base on the "
        "eastern bank, secure your power supply and destroy the node before "
        "reinforcements arrive.",
    "ОСНОВНАЯ ЦЕЛЬ\\nУничтожить командный узел\\n\\n"
    "ДОПОЛНИТЕЛЬНАЯ\\nСохранить мост\\n\\nВРЕМЯ\\n06:30":
        "PRIMARY OBJECTIVE\\nDestroy the command node\\n\\n"
        "SECONDARY\\nPreserve the bridge\\n\\nTIME\\n06:30",
    "АДМИРАЛ ВАРД:\\n«Командующий, сканеры фиксируют движение бронеколонн. "
    "Не давайте им закрепиться у моста. Воздушный коридор будет открыт на "
    "три минуты.»":
        "ADMIRAL WARD:\\n\\\"Commander, scanners show armoured columns on the "
        "move. Do not let them dig in at the bridge. The air corridor will be "
        "open for three minutes.\\\"",
    "КАНАЛ\\nВаршава-01\\n\\nШИФРОВАНИЕ\\nАктивно\\n\\nЗАДЕРЖКА\\n18 мс":
        "CHANNEL\\nWarsaw-01\\n\\nENCRYPTION\\nActive\\n\\nLATENCY\\n18 ms",
    "Загрузка театра боевых действий. Синхронизация данных командования, "
    "маршрутов снабжения и тактической разведки.":
        "Loading the theatre of operations. Synchronising command data, supply "
        "routes and tactical reconnaissance.",
    "КАРТА\\nПепел столицы\\n\\nФРАКЦИЯ\\nСССР\\n\\nСОСТОЯНИЕ\\nСинхронизация":
        "MAP\\nAshes of the Capital\\n\\nFACTION\\nUSSR\\n\\n"
        "STATUS\\nSynchronising",

    # --- pause / results ----------------------------------------------------
    "ОПЕРАЦИЯ\\nПепел столицы\\n\\nВРЕМЯ БОЯ\\n18:42\\n\\n"
    "СОХРАНЕНИЕ\\nАвтоматическое":
        "OPERATION\\nAshes of the Capital\\n\\nBATTLE TIME\\n18:42\\n\\n"
        "SAVING\\nAutomatic",
    "Командный узел противника уничтожен. Мост и плацдарм удержаны. Театр "
    "операций открыт для следующей фазы наступления.":
        "The enemy command node is destroyed. The bridge and beachhead are "
        "held. The theatre is open for the next phase of the offensive.",
    "РЕЗУЛЬТАТ\\nПобеда СССР\\n\\nВРЕМЯ\\n24:18\\n\\nПОТЕРИ\\nПриемлемые":
        "RESULT\\nUSSR Victory\\n\\nTIME\\n24:18\\n\\nLOSSES\\nAcceptable",

    # --- encyclopedia / tech tree ------------------------------------------
    "ТЕСЛА-ТАНК\\nТяжёлая бронированная единица СССР с цепной электрической "
    "пушкой.\\n\\nСТРАЖ\\nУниверсальная боевая машина Альянса с модульной "
    "защитой.\\n\\nТЕМПОРАЛЬНЫЙ СТРАЖ\\nПередовая единица Хронолегиона.":
        "TESLA TANK\\nHeavily armoured USSR unit with a chain lightning "
        "cannon.\\n\\nGUARDIAN\\nVersatile Alliance combat vehicle with "
        "modular protection.\\n\\nTEMPORAL GUARDIAN\\n"
        "Advanced Chronolegion unit.",
    "КАТЕГОРИЯ\\nБоевые единицы\\n\\nЗАПИСЕЙ\\n128\\n\\nФИЛЬТР\\nВсе фракции":
        "CATEGORY\\nCombat units\\n\\nENTRIES\\n128\\n\\nFILTER\\nAll factions",
    "КОМАНДНЫЙ ЦЕНТР → БАРАКИ → ВОЕННЫЙ ЗАВОД\\n\\n"
    "ВОЕННЫЙ ЗАВОД → ТЕСЛА-ЛАБОРАТОРИЯ → ТЕСЛА-ТАНК\\n\\n"
    "ЭЛЕКТРОСТАНЦИЯ → РАДАР → СПУТНИКОВАЯ СВЯЗЬ":
        "COMMAND CENTRE → BARRACKS → WAR FACTORY\\n\\n"
        "WAR FACTORY → TESLA LAB → TESLA TANK\\n\\n"
        "POWER PLANT → RADAR → SATELLITE UPLINK",
    "ФРАКЦИЯ\\nСССР\\n\\nОТКРЫТО\\n18 / 46\\n\\nОЧКИ\\n3":
        "FACTION\\nUSSR\\n\\nUNLOCKED\\n18 / 46\\n\\nPOINTS\\n3",

    # --- mods ---------------------------------------------------------------
    "РАСШИРЕННЫЕ КАРТЫ — включено\\nНовые сценарии и таблицы баланса.\\n\\n"
    "ТАКТИЧЕСКИЕ ПОРТРЕТЫ — включено\\nДополнительные портреты "
    "командующих.\\n\\nЭКСПЕРИМЕНТАЛЬНЫЙ БАЛАНС — выключено":
        "EXTENDED MAPS — enabled\\nNew scenarios and balance tables.\\n\\n"
        "TACTICAL PORTRAITS — enabled\\nAdditional commander "
        "portraits.\\n\\nEXPERIMENTAL BALANCE — disabled",
    "АКТИВНЫХ МОДОВ\\n2\\n\\nСОВМЕСТИМОСТЬ\\nПроверена\\n\\n"
    "ПЕРЕЗАПУСК\\nНе требуется":
        "ACTIVE MODS\\n2\\n\\nCOMPATIBILITY\\nVerified\\n\\n"
        "RESTART\\nNot required",

    # --- alerts -------------------------------------------------------------
    "КРЕДИТЫ 18 300     ЭНЕРГИЯ 1020 / 1180\\n\\nПРОИЗВОДСТВО\\n"
    "Апокалипсис — 89%\\nМиГ «Кузнец» — 31%\\n\\nТАКТИЧЕСКОЕ СОБЫТИЕ\\n"
    "Враг атакует южный мост. Активируйте заградительный огонь.":
        "CREDITS 18,300     POWER 1020 / 1180\\n\\nPRODUCTION\\n"
        "Apocalypse — 89%\\nMiG \\\"Kuznets\\\" — 31%\\n\\nTACTICAL EVENT\\n"
        "The enemy is attacking the south bridge. Activate barrage fire.",
    "МИНИ-КАРТА\\nЮг: красный контакт\\n\\nВЫДЕЛЕНО\\n4 × Тесла-танк\\n\\n"
    "УГРОЗА\\nВысокая":
        "MINIMAP\\nSouth: red contact\\n\\nSELECTED\\n4 × Tesla Tank\\n\\n"
        "THREAT\\nHigh",
    "Обнаружены самолёты Альянса. Разверните ПВО, переместите мобильные "
    "генераторы и защитите Тесла-лабораторию.":
        "Alliance aircraft detected. Deploy AA defences, relocate mobile "
        "generators and protect the Tesla Lab.",
    "КОНТАКТОВ\\n12\\n\\nДО ПОДЛЁТА\\n00:32\\n\\nПВО\\n4 батареи":
        "CONTACTS\\n12\\n\\nTIME TO ARRIVAL\\n00:32\\n\\nAA\\n4 batteries",
    "КРЕДИТЫ 16 800     ЭНЕРГИЯ 720 / 840\\n\\nФЛОТ\\n"
    "Авианосец «Свобода» — готов\\nЭсминец ×3\\n\\nЗАДАЧА: провести конвой "
    "через пролив и подавить береговые батареи.":
        "CREDITS 16,800     POWER 720 / 840\\n\\nFLEET\\n"
        "Carrier \\\"Liberty\\\" — ready\\nDestroyer ×3\\n\\nOBJECTIVE: escort "
        "the convoy through the strait and suppress the coastal batteries.",
    "МОРСКАЯ КАРТА\\nКвадрат D-4\\n\\nКОНВОЙ\\n5 / 6 кораблей\\n\\n"
    "ПОГОДА\\nШторм":
        "NAVAL MAP\\nGrid D-4\\n\\nCONVOY\\n5 / 6 ships\\n\\n"
        "WEATHER\\nStorm",
    "КРЕДИТЫ 11 900     ЭНЕРГИЯ 680 / 820\\n\\nАВИАКРЫЛО\\n"
    "Истребитель «Копьё» ×6\\nБомбардировщик «Гром» ×2\\n\\nЗАДАЧА: "
    "уничтожить радар противника и удержать воздушный коридор.":
        "CREDITS 11,900     POWER 680 / 820\\n\\nAIR WING\\n"
        "Spear Fighter ×6\\nThunder Bomber ×2\\n\\nOBJECTIVE: destroy the "
        "enemy radar and hold the air corridor.",
    "ВОЗДУШНАЯ КАРТА\\nВысота 8 000 м\\n\\nТОПЛИВО\\n76%\\n\\n"
    "КОНТАКТЫ\\n3 эскадрильи":
        "AIR MAP\\nAltitude 8,000 m\\n\\nFUEL\\n76%\\n\\n"
        "CONTACTS\\n3 squadrons",
    "ХРОНОЭНЕРГИЯ 94 / 100\\n\\nВРЕМЕННОЙ ЯКОРЬ\\nСтабилен\\n\\n"
    "ВЫБЕРИТЕ ЗОНУ НАЗНАЧЕНИЯ\\nПосле активации цель будет выведена из "
    "текущей временной линии на 12 секунд.":
        "CHRONO ENERGY 94 / 100\\n\\nTEMPORAL ANCHOR\\nStable\\n\\n"
        "SELECT TARGET ZONE\\nOnce activated the target will be removed from "
        "the current timeline for 12 seconds.",
    "РАДИУС\\n420 м\\n\\nПЕРЕЗАРЯДКА\\n00:18\\n\\n"
    "РИСК ПАРАДОКСА\\nУмеренный":
        "RADIUS\\n420 m\\n\\nCOOLDOWN\\n00:18\\n\\n"
        "PARADOX RISK\\nModerate",
    "Гао строит войну на дисциплине, темпе производства и контроле узлов "
    "снабжения. Его штаб ведёт наступление от тихоокеанского побережья к "
    "промышленному сердцу Европы.":
        "Gao builds his war on discipline, production tempo and control of "
        "supply nodes. His staff drives the offensive from the Pacific coast "
        "to the industrial heart of Europe.",
    "ДОКТРИНА\\nНефритовый молот\\n\\nБОНУС\\nУскоренное производство\\n\\n"
    "КАМПАНИЯ\\n01 / 08":
        "DOCTRINE\\nJade Hammer\\n\\nBONUS\\nAccelerated production\\n\\n"
        "CAMPAIGN\\n01 / 08",

    # --- decorative / locale-neutral ---------------------------------------
    # These are ASCII art, numbers, format-only patterns or brand names.
    # They must stay byte-identical, so they map to themselves.
    "V": "V",
    "RED ALERT 4": "RED ALERT 4",
    "RA4": "RA4",
    "==========  V  ==========": "==========  V  ==========",
    "=======   ==   ==   ==": "=======   ==   ==   ==",
    "================================================":
        "================================================",
    "================": "================",
    "{0} / {1}": "{0} / {1}",
    "{0}  {1}%": "{0}  {1}%",
    "04\\n\\n17\\n\\n26\\n\\n49": "04\\n\\n17\\n\\n26\\n\\n49",
    "00:18:42": "00:18:42",
}


def escape_for_po(text: str) -> str:
    """Quote a value the way .po expects (only quotes need escaping here)."""
    return text.replace('"', '\\"')


def main() -> int:
    if not PO_PATH.exists():
        print("not found: %s" % PO_PATH, file=sys.stderr)
        return 1

    text = PO_PATH.read_text(encoding="utf-8")

    filled = 0
    missing = {}

    def replace(match: "re.Match[str]") -> str:
        nonlocal filled
        head, source, current = match.group(1), match.group(2), match.group(3)
        if current.strip():
            return match.group(0)          # already translated, leave alone
        en = TRANSLATIONS.get(source)
        if en is None:
            missing[source] = missing.get(source, 0) + 1
            return match.group(0)
        filled += 1
        return '%smsgid "%s"\nmsgstr "%s"' % (head, source, escape_for_po(en))

    # Capture the msgctxt line (if any) plus the msgid/msgstr pair.
    pattern = re.compile(
        r'((?:msgctxt "(?:[^"\\]|\\.)*"\n)?)msgid "((?:[^"\\]|\\.)*)"\n'
        r'msgstr "((?:[^"\\]|\\.)*)"'
    )
    new_text = pattern.sub(replace, text)

    PO_PATH.write_text(new_text, encoding="utf-8")

    print("filled %d entries" % filled)
    if missing:
        print("UNTRANSLATED (%d distinct):" % len(missing))
        for src in missing:
            print("  %r" % src)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
