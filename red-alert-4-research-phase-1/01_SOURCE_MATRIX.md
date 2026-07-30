# 01. Матрица источников

## A. Официальный исходный код Generals / Zero Hour

Источник: https://github.com/electronicarts/CnC_Generals_Zero_Hour

Статус:

- официальный репозиторий Electronic Arts;
- опубликован под GPL-3.0 с дополнительными условиями EA;
- архивирован 27 февраля 2025 года;
- содержит деревья `Generals` и `GeneralsMD`;
- не является современным готовым SDK: исходный build рассчитан на Visual Studio 6 и зависит от ряда старых либо проприетарных библиотек.

Полезность:

- границы GameClient / GameLogic / GameNetwork / Common;
- объектная модель и модульные поведения;
- команды, кадры синхронизации и сетевой транспорт;
- AI, pathfinding, weapons, armor, scripting, victory conditions;
- сохранение игрового состояния и инфраструктура данных.

Ограничение:

- код нельзя копировать в закрытый коммерческий Unreal-модуль;
- допустимо изучать идеи, границы подсистем, инварианты и алгоритмические требования, после чего писать независимую реализацию.

## B. Официальный C&C Modding Support

Источник: https://github.com/electronicarts/CnC_Modding_Support

RA3: https://github.com/electronicarts/CnC_Modding_Support/tree/main/Red%20Alert%203

Статус:

- официальный репозиторий EA;
- архивирован 15 августа 2025 года;
- для RA3 содержит `Libraries`, `Schemas`, `Shaders`, `Xml`, `RA3Music.h`;
- полного C++-кода RA3 не содержит.

Полезность:

- формальные XSD-схемы;
- структура игровых объектов и их наследования;
- экономика, зависимости и технологические уровни;
- команды, abilities, upgrades, AI и multiplayer settings;
- визуальные состояния, FX и shader feature taxonomy;
- структура campaign/skirmish scripts.

Ограничение:

- это не свободная библиотека готовых данных для коммерческой игры;
- значения, названия, фракции, ссылки на арт и баланс нельзя механически переносить в продукт.

## C. Официальные правила моддинга EA

Источник: https://www.ea.com/games/command-and-conquer/command-and-conquer-remastered/news/modding-faq

Критические положения для проекта:

- EA перечисляет полный опубликованный GPL-код только для Red Alert, Tiberian Dawn, Renegade, Generals и Zero Hour;
- моды с материалами C&C должны быть бесплатными и некоммерческими;
- нельзя создавать впечатление официальной связи с EA;
- использование торговых марок, логотипов и музыки ограничено;
- для публичного мода требуется дисклеймер о том, что EA его не поддерживает.

## D. RA3 Mod SDK и art/campaign packs

Вторичный каталог: https://www.cnclabs.com/downloads/redalert3/modding-and-mapping.aspx

Полезность:

- Mod SDK v3;
- World Builder;
- campaign source files;
- W3X/TGA/XML/3ds Max source packs;
- практическое понимание старого content pipeline.

Статус доверия:

- использовать как навигацию и зеркало;
- юридические выводы проверять по материалам EA;
- бинарные загрузки не включать в производственный репозиторий без отдельной проверки происхождения.

## E. Активный community fork GeneralsGameCode

Источник: https://github.com/TheSuperHackers/GeneralsGameCode

Полезность:

- современная навигация по старому коду;
- перенос на Visual Studio 2022/C++20;
- обсуждения детерминизма, replays, desync и кроссплатформенной математики;
- полезен для поиска известных проблем исходной архитектуры.

Ограничение:

- это не источник истины о первоначальном поведении;
- изменения community fork должны сверяться с официальным snapshot EA;
- GPL-код по-прежнему не переносится в закрытый Unreal runtime.
