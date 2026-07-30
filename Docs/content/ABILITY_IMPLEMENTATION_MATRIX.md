# Ability Implementation Matrix

Abilities are loaded from the bible as text descriptions. Each unit card has
an `abilities` array with the canonical description. Runtime GAS implementation
is pending — the data is loaded and accessible via `EntityDef::Abilities`.

| Unit ID | Abilities (from bible) | Runtime Status |
|---------|------------------------|----------------|
| SU_RubezhRifleman | «Окопаться»: 2 сек подготовки, +35% защита и -40% скорость до отмены | Data loaded, GAS pending |
| SU_ZapalGrenadier | «Термобарический заряд»: выбивает гарнизон и поджигает здание; 28 сек | Data loaded, GAS pending |
| SU_ZaslonAATeam | «Воздушная засада»: маскируется на 6 сек и получает +40% первый залп | Data loaded, GAS pending |
| SU_MasterEngineer | «Полевой ремонт»: восстанавливает 300 HP технике за 8 сек; «Захват»: занимает нейтральные и вражеские здания | Data loaded, GAS pending |
| SU_RazryadTrooper | «Перегрузка»: цепная молния по 4 целям; 30 сек | Data loaded, GAS pending |
| SU_VektorOfficer | «Приказ №1»: союзная пехота в радиусе получает +20% урон и иммунитет к подавлению на 10 сек; 35 сек; Пассивно ускоряет получение Мобилизации | Data loaded, GAS pending |
| SU_BogatyrOreCarrier | «Аварийная броня»: на 8 сек получает -40% входящего урона; 50 сек | Data loaded, GAS pending |
| SU_RysScout | «Прыжок через препятствие»: короткий прыжок, 18 сек | Data loaded, GAS pending |
| SU_GranitMBT | «Таран»: ускоряется и отбрасывает лёгкую технику; 26 сек | Data loaded, GAS pending |
| SU_ZarevoMLRS | «Огненный квадрат»: залп по большой зоне, оставляет горение; 38 сек | Data loaded, GAS pending |
| SU_GromoboyRam | «Разряд по земле»: конусный электрический удар и краткое оглушение техники; 32 сек | Data loaded, GAS pending |
| SU_VoevodaHeavyTank | «Осадный режим»: -60% скорость, +30% дальность и броня; 4 сек развёртывания | Data loaded, GAS pending |
| SU_KrechetInterceptor | «Форсаж»: +40% скорость на 6 сек; 25 сек | Data loaded, GAS pending |
| SU_KorshunGunship | «Высадка»: перевозит 6 пехотинцев; «Круг огня»: зависает и усиливает огонь на 8 сек | Data loaded, GAS pending |
| SU_GromadaAirship | «Полный газ»: +60% скорость на 10 сек, затем получает 20% урона; 50 сек | Data loaded, GAS pending |
| SU_BuranPatrolBoat | «Электросеть»: ставит электрическую мину на воде; 25 сек | Data loaded, GAS pending |
| SU_MorokSubmarine | «Беззвучный ход»: повышенная маскировка на 12 сек; 35 сек | Data loaded, GAS pending |
| SU_SvyatogorCruiser | «Заградительный залп»: 6 ракет по широкой области; 50 сек | Data loaded, GAS pending |
| SU_Hero_Morozova | «Поле подавления»: враги в области теряют скорость и точность; 40 сек; «Командный импульс»: союзники мгновенно получают 20 Мобилизации; 60 сек (+1 more) | Data loaded, GAS pending |
| AL_SentinelRifleman | «Светошумовой заряд»: снижает точность врага; 24 сек | Data loaded, GAS pending |
| AL_LancerTeam | «Лазерная метка»: цель получает +20% урона от всех союзников 8 сек; 30 сек | Data loaded, GAS pending |
| AL_FieldEngineer | «Ремонтный рой»: дроны чинят технику на расстоянии; 35 сек | Data loaded, GAS pending |
| AL_LongwatchSniper | «Скрытый наблюдатель»: маскируется неподвижно и увеличивает обзор; 4 сек | Data loaded, GAS pending |
| AL_LifelineMedic | «Стабилизация»: возвращает союзной пехоте 40% HP за 6 сек; 30 сек | Data loaded, GAS pending |
| AL_FrostlineSpecialist | «Полная заморозка»: обездвиживает цель на 4 сек; 34 сек | Data loaded, GAS pending |
| AL_PioneerHarvester | «Развернуть форпост»: становится малой ремонтно-строительной площадкой; повторное сворачивание 10 сек | Data loaded, GAS pending |
| AL_KestrelScout | «Активный скан»: раскрывает скрытые цели; 20 сек | Data loaded, GAS pending |
| AL_BulwarkMBT | «Целеуказатель»: снижает броню цели на 20% на 8 сек; 28 сек | Data loaded, GAS pending |
| AL_OracleArtillery | «Синхронный залп»: мощный выстрел после 3 сек наведения; 36 сек | Data loaded, GAS pending |
| AL_RefractionTank | «Оптическая маскировка»: в неподвижности становится невидимым; первый выстрел +35% урон | Data loaded, GAS pending |
| AL_WardShieldCarrier | «Проекция»: создаёт направленный щит на 12 сек; 32 сек | Data loaded, GAS pending |
| AL_CitadelTank | «Активная защита»: перехватывает 6 ракет/снарядов; 40 сек | Data loaded, GAS pending |
| AL_ShrikeInterceptor | «Перехват»: мгновенно ускоряется к выбранной воздушной цели; 24 сек | Data loaded, GAS pending |
| AL_VectorVTOL | «Вертикальная засада»: зависает за рельефом и получает +25% первый залп | Data loaded, GAS pending |
| AL_NightveilBomber | «Режим тени»: не обнаруживается обычным радаром до сброса; 45 сек | Data loaded, GAS pending |
| AL_MantaPatrolCraft | «Радиоподавление»: отключает оружие одного корабля на 5 сек; 28 сек | Data loaded, GAS pending |
| AL_ResoluteDestroyer | «Сонарный импульс»: раскрывает подлодки в области; 30 сек | Data loaded, GAS pending |
| AL_HorizonCarrier | «Полный авиапакет»: запускает 8 дронов по области; 55 сек | Data loaded, GAS pending |
| AL_Hero_Hart | «Призрачный протокол»: полная маскировка на 10 сек; 45 сек; «Взлом»: временно отключает вражеское здание или технику; 50 сек (+1 more) | Data loaded, GAS pending |
| CO_QianweiRifleman | «Строй»: рядом с двумя бойцами «Цяньвэй» получает +15% защиты | Data loaded, GAS pending |
| CO_VajraLancer | «Импульсный выпад»: короткий рывок и отключение лёгкой техники на 2 сек; 24 сек | Data loaded, GAS pending |
| CO_JieTechnician | «Связать узел»: временно подключает изолированное здание к сети; «Ремонтный рой»: ремонт техники | Data loaded, GAS pending |
| CO_ShengongMarksman | «Пробой щита»: следующий выстрел игнорирует щит; 28 сек | Data loaded, GAS pending |
| CO_SanjivaniMedic | «Защитная оболочка»: даёт 200 щита на 10 сек; 32 сек | Data loaded, GAS pending |
| CO_RakshaGuard | «Отражение»: 4 сек отражает 40% дальнего урона; 35 сек | Data loaded, GAS pending |
| CO_YuanCollector | «Энергосвязь»: рядом со зданиями даёт +10 Синхронизации | Data loaded, GAS pending |
| CO_KamakiriWalker | «Стенной шаг»: преодолевает малые уступы и баррикады | Data loaded, GAS pending |
| CO_QinglongMBT | «Сцепление щитов»: соединяет щиты соседних танков, распределяя урон | Data loaded, GAS pending |
| CO_MonsoonArtillery | «Фронт муссона»: разворачивается, получает +25% дальности и разделяет снаряд на три подбоеприпаса | Data loaded, GAS pending |
| CO_SeimonShieldCarrier | «Купол»: создаёт круговой щит 12 сек; 38 сек; Пассивно +10 Синхронизации рядом с 4+ юнитами | Data loaded, GAS pending |
| CO_AiravataWalker | «Небесный прыжок»: перепрыгивает линию фронта и наносит удар при посадке; 42 сек | Data loaded, GAS pending |
| CO_TianmenFortress | «Командный режим»: останавливается, +20 Синхронизации и ремонтирует союзников | Data loaded, GAS pending |
| CO_KawasemiDrone | «Сетевой маяк»: повышает Синхронизацию видимых союзников | Data loaded, GAS pending |
| CO_LeiheGunship | «Защитное крыло»: даёт союзной группе 150 щита; 35 сек | Data loaded, GAS pending |
| CO_AgnipakshaBomber | «Возрождение»: один раз за жизнь при смертельном уроне возвращается на базу с 25% HP | Data loaded, GAS pending |
| CO_KazekiriCorvette | «Режущий манёвр»: рывок вдоль цели, снижает её точность | Data loaded, GAS pending |
| CO_XuanwuCruiser | «Стабилизированный выстрел»: пробивает несколько целей по линии; 38 сек | Data loaded, GAS pending |
| CO_SamudraCarrier | «Погружённый запуск»: выпускает смешанный рой, не раскрываясь 6 сек | Data loaded, GAS pending |
| CO_Hero_Mei | «Совершенный строй»: мгновенно даёт группе максимальные бонусы построения на 12 сек; «Перенос щита»: перенаправляет щиты союзников к выбранной цели; 40 сек (+1 more) | Data loaded, GAS pending |
| CH_ResonanceRifleman | Каждый четвёртый залп повторяется через 0.6 сек с 50% урона | Data loaded, GAS pending |
| CH_PunctureLancer | «Фазовый шаг»: становится неуязвимым на 1 сек и проходит сквозь юниты; 22 сек, 8 стабильности | Data loaded, GAS pending |
| CH_CausalityEngineer | «Перемотка ремонта»: возвращает зданию состояние 6 сек назад; 40 сек, 15 стабильности | Data loaded, GAS pending |
| CH_ReversalMedic | «Возврат состояния»: союзник возвращает HP, имевшиеся 5 сек назад; 32 сек, 12 стабильности | Data loaded, GAS pending |
| CH_AporiaSniper | «Отложенная смерть»: урон срабатывает через 4 сек и удваивается, если цель получает второй выстрел; 30 сек | Data loaded, GAS pending |
| CH_CensorOperative | «Обнуление»: отключает активные способности цели на 8 сек; 38 сек, 18 стабильности; Может маскироваться вне боя | Data loaded, GAS pending |
| CH_ProbabilistHarvester | «Квантовый возврат»: телепортируется к переработчику; 45 сек, 15 стабильности | Data loaded, GAS pending |
| CH_ParallaxScout | «Скачок»: телепорт на короткую дистанцию; 12 сек, 6 стабильности | Data loaded, GAS pending |
| CH_TimelineTank | «Временной панцирь»: 6 сек записывает урон, затем возвращает 40% потерянного HP; 34 сек, 14 стабильности | Data loaded, GAS pending |
| CH_DeltaDelayArtillery | «Поле задержки»: область замедляет врагов на 50% 8 сек; 40 сек, 18 стабильности | Data loaded, GAS pending |
| CH_PauseProjector | «Полный стазис»: выключает цель из боя на 5 сек; 42 сек, 22 стабильности | Data loaded, GAS pending |
| CH_EraEngine | «Послеобраз»: создаёт копию с 45% характеристик на 15 сек; 55 сек, 30 стабильности | Data loaded, GAS pending |
| CH_GapInterceptor | «Разрыв курса»: мгновенно меняет позицию за целью; 22 сек, 8 стабильности | Data loaded, GAS pending |
| CH_TrailGunship | «Ложный рой»: создаёт 3 неатакующих копии, сбивающих наведение; 34 сек, 12 стабильности | Data loaded, GAS pending |
| CH_CriticalPointBomber | «Обратная волна»: после взрыва враги притягиваются к центру; 48 сек, 24 стабильности | Data loaded, GAS pending |
| CH_IsobathFrigate | «Отметка прилива»: помечает область; союзные корабли получают +15% скорость | Data loaded, GAS pending |
| CH_BathysSubmarine | «Глубинный скачок»: телепорт под водой; 30 сек, 14 стабильности | Data loaded, GAS pending |
| CH_AttractorArk | «Морской портал»: телепортирует до 6 союзных кораблей к себе; 65 сек, 35 стабильности | Data loaded, GAS pending |
| CH_Hero_Voss | «Архив состояния»: записывает состояние группы и может вернуть его в течение 12 сек; «Запрет события»: отменяет одно применение вражеской способности; 60 сек (+1 more) | Data loaded, GAS pending |

**Total: 78 units with abilities**
