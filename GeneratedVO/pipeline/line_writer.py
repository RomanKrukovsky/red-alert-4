"""Generate voice_lines.json with all event text variants for the test package.

Each (VoiceId, Event) gets 1 line for test (will be expanded to full variants after approval).

Event rules:
- 1 line per (VoiceId, Event) in test
- Length: 0.7–3 seconds spoken at cfg=2.0 (roughly 5-20 chars/sec Russian)
- Russian natural, no faction name repetition
- Numbers/abbreviations as spoken form
- Use ё where stress ambiguous
"""
import json
import os

OUT = "/Users/romanmolodyko/Documents/red-alert-4/GeneratedVO/voice_lines.json"

# Events per voice type
EVA_EVENTS = ["Game.Start", "Base.UnderAttack", "Unit.Ready", "Player.Victory", "Resources.Low"]
RIFLEMAN_EVENTS = ["Selected", "Move", "Attack", "EnemyDestroyed", "Idle"]
TANK_EVENTS = ["Selected", "Attack", "Attack.Building", "Damaged", "CriticalDamage"]
FIGHTER_EVENTS = ["Selected", "Attack.Air", "Ability.Activate", "EnemyDestroyed", "Death"]
CAPTAIN_EVENTS = ["Selected", "Move", "Attack", "CannotComply", "Death"]

# Style prefix for emotional events (Tone Guide from TЗ)
STYLE_PREFIX = {
    "Game.Start": "(calm command announcement) ",
    "Base.UnderAttack": "(urgent command) ",
    "Unit.Ready": "(alert ready) ",
    "Player.Victory": "(restrained triumph) ",
    "Resources.Low": "(measured warning) ",
    "Selected": "(alert ready concise) ",
    "Move": "(confident acknowledgement focused) ",
    "Attack": "(controlled aggression combat) ",
    "Attack.Building": "(controlled aggression combat) ",
    "Attack.Air": "(focused combat intensity) ",
    "Ability.Activate": "(sharp focused) ",
    "EnemyDestroyed": "(controlled satisfaction) ",
    "Idle": "(relaxed in character) ",
    "Damaged": "(strained but intelligible urgent) ",
    "CriticalDamage": "(severe urgency controlled panic) ",
    "Death": "(short final reaction) ",
    "CannotComply": "(firm concise refusal) ",
    "Spawn": "(alert ready) ",
}

# Per-(faction, event) text variants
LINES = []

def add(voice_id, faction, event, text, variant=1):
    LINES.append({
        "voice_id": voice_id,
        "faction": faction,
        "event": event,
        "variant": variant,
        "text": text,
        "style_prefix": STYLE_PREFIX.get(event, ""),
        "control_style": STYLE_PREFIX.get(event, "").strip("() ") if STYLE_PREFIX.get(event) else ""
    })

# ============ EVA ============
# Soviet EVA
sov = [
    ("Game.Start", "Внимание. Штаб фронта к работе приступил."),
    ("Base.UnderAttack", "Тревога. База под огнём противника."),
    ("Unit.Ready", "Подразделение готово к выдвижению."),
    ("Player.Victory", "Операция выполнена. Победа за нами."),
    ("Resources.Low", "Ресурсы на исходе. Сократите расход."),
]
for ev, txt in sov: add("EVA_Soviet", "Soviet", ev, txt)

# Alliance EVA
all_ = [
    ("Game.Start", "Командная сеть активирована. Готова к работе."),
    ("Base.UnderAttack", "Тревога. Наша база под ударом."),
    ("Unit.Ready", "Боевая единица готова к развёртыванию."),
    ("Player.Victory", "Миссия завершена. Превосходство подтверждено."),
    ("Resources.Low", "Внимание. Запас ресурсов снижается."),
]
for ev, txt in all_: add("EVA_Alliance", "Alliance", ev, txt)

# Coalition EVA
coa = [
    ("Game.Start", "Центр командования на связи. Операция начата."),
    ("Base.UnderAttack", "Тревога. Объект под огнём противника."),
    ("Unit.Ready", "Подразделение в полной готовности."),
    ("Player.Victory", "Стратегическая цель достигнута."),
    ("Resources.Low", "Ресурсы ограничены. Координируйте расход."),
]
for ev, txt in coa: add("EVA_Coalition", "Coalition", ev, txt)

# Chrono EVA
chr = [
    ("Game.Start", "Цикл инициализирован. Координатор на связи."),
    ("Base.UnderAttack", "Аномалия. Объект под воздействием."),
    ("Unit.Ready", "Структура развёрнута. Готова к координации."),
    ("Player.Victory", "Временная линия зафиксирована. Успех подтверждён."),
    ("Resources.Low", "Ресурс исчерпывается. Замедление процессов."),
]
for ev, txt in chr: add("EVA_Chrono", "Chrono", ev, txt)

# ============ Riflemen ============
def rifleman(faction, prefix):
    p = prefix  # Faction tag for filename
    f = faction
    set_ = [
        ("Selected", "Пехота на позиции."),
        ("Move", "Выдвигаюсь."),
        ("Attack", "Огонь по противнику."),
        ("EnemyDestroyed", "Цель нейтрализована."),
        ("Idle", "Жду приказа."),
    ]
    for ev, txt in set_: add(f"{p}_Rifleman", f, ev, txt)

rifleman("Soviet", "USSR")
rifleman("Alliance", "Alliance")
rifleman("Coalition", "Coalition")
rifleman("Chrono", "Chrono")

# ============ MainTank ============
def tank(faction, prefix):
    p = prefix
    f = faction
    set_ = [
        ("Selected", "Танк в строю."),
        ("Attack", "Орудие к бою. Огонь."),
        ("Attack.Building", "Заградительный огонь по укреплениям."),
        ("Damaged", "Попадание. Маневрирую."),
        ("CriticalDamage", "Броня пробита. Аварийный режим."),
    ]
    for ev, txt in set_: add(f"{p}_MainTank", f, ev, txt)

tank("Soviet", "USSR")
tank("Alliance", "Alliance")
tank("Coalition", "Coalition")
tank("Chrono", "Chrono")

# ============ Fighter ============
def fighter(faction, prefix):
    p = prefix
    f = faction
    set_ = [
        ("Selected", "Пилот в кабине."),
        ("Attack.Air", "Воздушный бой. Атакую ведущего."),
        ("Ability.Activate", "Активирую систему."),
        ("EnemyDestroyed", "Цель сбита."),
        ("Death", "Катапультируюсь."),
    ]
    for ev, txt in set_: add(f"{p}_Fighter", f, ev, txt)

fighter("Soviet", "USSR")
fighter("Alliance", "Alliance")
fighter("Coalition", "Coalition")
fighter("Chrono", "Chrono")

# ============ Captain ============
def captain(faction, prefix):
    p = prefix
    f = faction
    set_ = [
        ("Selected", "Капитан на мостике."),
        ("Move", "Курс на позицию."),
        ("Attack", "Огонь по кораблю противника."),
        ("CannotComply", "Приказ невыполним."),
        ("Death", "Корабль теряем."),
    ]
    for ev, txt in set_: add(f"{p}_Captain", f, ev, txt)

captain("Soviet", "USSR")
captain("Alliance", "Alliance")
captain("Coalition", "Coalition")
captain("Chrono", "Chrono")

OUTPUT = {
    "schema_version": "1.0",
    "language": "ru-RU",
    "test_mode": True,
    "total_lines": len(LINES),
    "lines": LINES
}

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(OUTPUT, f, ensure_ascii=False, indent=2)
print(f"Wrote {OUT} with {len(LINES)} test lines")
print(f"Per voice: 1 line per event, 5 events per voice")
print(f"Voices: {len(set(l['voice_id'] for l in LINES))}")
