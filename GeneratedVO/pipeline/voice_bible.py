"""Generate voice_bible.json with all VoiceId definitions for the test package.

20 voices: 4 EVA + 4 Rifleman + 4 MainTank + 4 Fighter + 4 Captain.
Each voice has: id, faction, role, gender_age, control_instruction (EN),
ru_pacing (RU), prosody_caps, forbidden_traits, sample_texts (3 RU anchors).
"""
import json
import os

OUT = "/Users/romanmolodyko/Documents/red-alert-4/GeneratedVO/voice_bible.json"

EVA_CONTROL = {
    "Soviet": "Authoritative mature female military command voice, low contralto, precise Russian diction, restrained emotion, firm concise delivery, calm under pressure.",
    "Alliance": "Professional female tactical AI voice, clean and intelligent, confident, modern, precise, slightly warm, medium-fast pace, excellent Russian diction.",
    "Coalition": "Calm disciplined female strategic command voice, elegant and precise, controlled emotion, measured pace, subtle authority, clear standard Russian.",
    "Chrono": "Androgynous timeless command voice, calm and unsettling, extremely precise, restrained emotion, deliberate micro-pauses, clear Russian, no baked-in audio effects.",
}

UNIT_CONTROL = {
    "Rifleman": {
        "Soviet": "Young Russian male soldier voice, raw infantry energy, short clipped Russian phrases, confident, slightly hoarse from field duty, medium pace.",
        "Alliance": "Young adult male soldier voice, modern professional infantry, energetic and clear, English-influenced Russian phrasing, fast medium pace.",
        "Coalition": "Calm disciplined male soldier voice, disciplined, measured, precise short Russian phrases, focused, neutral continental accent.",
        "Chrono": "Eerily calm young male soldier voice, flat affect, precise diction, minimal emotion, deliberate pacing, almost inhuman steadiness.",
    },
    "MainTank": {
        "Soviet": "Heavyset Russian male tank commander voice, deep chest voice, low rumble, confident and unhurried, mature veteran, slow medium pace.",
        "Alliance": "Professional male tank commander voice, confident and precise, medium-low pitch, modern technical vocabulary, medium pace.",
        "Coalition": "Calm disciplined male armor officer voice, lower register, methodical, restrained, precise short phrases, medium pace.",
        "Chrono": "Anomalous calm male commander voice, resonant low pitch, perfect diction, no emotion, unnatural steadiness, slow deliberate pace.",
    },
    "Fighter": {
        "Soviet": "Young Russian male combat pilot voice, energetic and focused, short commanding bursts, slight hoarseness, fast medium pace.",
        "Alliance": "Young adult male fighter pilot voice, sharp and confident, fast medium pace, technical slang, energetic delivery.",
        "Coalition": "Calm disciplined male pilot voice, controlled and precise, fast medium pace, minimal filler words, professional.",
        "Chrono": "Eerily calm male pilot voice, even pace, flat affect, perfect diction, minimal emotion, almost mechanical steadiness.",
    },
    "Captain": {
        "Soviet": "Mature Russian male naval officer voice, deep register, confident and commanding, experienced seaman, slow medium pace with gravitas.",
        "Alliance": "Professional male naval commander voice, clear and authoritative, medium register, crisp diction, medium pace.",
        "Coalition": "Calm disciplined male captain voice, lower register, measured and precise, restrained authority, medium-slow pace.",
        "Chrono": "Eerily calm male captain voice, deep resonant, no emotion, perfect cadence, slow deliberate pace, unsettling steadiness.",
    },
}

FACTION_TAG = {"Soviet": "USSR", "Alliance": "Alliance", "Coalition": "Coalition", "Chrono": "Chrono"}

VOICES = []
for faction in ["Soviet", "Alliance", "Coalition", "Chrono"]:
    # EVA
    VOICES.append({
        "id": f"EVA_{faction}",
        "faction": faction,
        "role": "EVA",
        "gender_age": "female 30-45" if faction != "Chrono" else "androgynous ageless",
        "control_instruction": EVA_CONTROL[faction],
        "ru_pacing": {
            "Soviet": "commanding, restrained",
            "Alliance": "professional, medium-fast",
            "Coalition": "elegant, measured",
            "Chrono": "calm, micro-pauses"
        }[faction],
        "prosody_caps": {
            "Soviet": "low pitch, firm cadence",
            "Alliance": "clean, slightly warm",
            "Coalition": "elegant, precise",
            "Chrono": "androgynous, deliberate pauses"
        }[faction],
        "forbidden_traits": {
            "Soviet": "hysteria, parody accent, theatrical screaming",
            "Alliance": "robotic monotone, valley girl",
            "Coalition": "caricature Asian accent, broken Russian",
            "Chrono": "digital glitch artifacts in audio, robotic monotone"
        }[faction],
        "sample_texts": [
            "Я — оператор командной связи. Докладываю обстановку.",
            "Подразделение, слушать мою команду. Выполнять.",
            "Контакт с противником. Запрашиваю поддержку."
        ]
    })
    # Units
    for role in ["Rifleman", "MainTank", "Fighter", "Captain"]:
        unit_id = f"{FACTION_TAG[faction]}_{role}"
        role_samples = {
            "Rifleman": [
                "Я пехотинец. Держу оборону.",
                "Подразделение, слушать мою команду. Выполнять.",
                "Контакт с противником. Запрашиваю поддержку."
            ],
            "MainTank": [
                "Танковый взвод на позиции. Готов к выдвижению.",
                "Экипаж, слушать мою команду. Работаем.",
                "Противник в зоне поражения. Открываю огонь."
            ],
            "Fighter": [
                "Пилот в воздухе. Готов к заданию.",
                "Ведомый, слушать мою команду. Работаем.",
                "Контакт с противником в воздухе. Атакую."
            ],
            "Captain": [
                "Капитан на мостике. Корабль в готовности.",
                "Экипаж, слушать мою команду. Манёвр по плану.",
                "Вражеский корабль на радаре. Открываю огонь."
            ]
        }
        VOICES.append({
            "id": unit_id,
            "faction": faction,
            "role": role,
            "gender_age": "male 25-40",
            "control_instruction": UNIT_CONTROL[role][faction],
            "ru_pacing": "energetic" if role == "Rifleman" else ("deep slow" if role == "MainTank" else ("fast sharp" if role == "Fighter" else "measured gravitas")),
            "prosody_caps": {
                "Rifleman": "short clipped phrases",
                "MainTank": "low rumble, heavy cadence",
                "Fighter": "energetic bursts",
                "Captain": "deep authoritative"
            }[role],
            "forbidden_traits": {
                "Soviet": "parody accent, theatrical screaming",
                "Alliance": "valley girl, surfer slang",
                "Coalition": "caricature Asian accent, broken Russian",
                "Chrono": "robotic monotone, digital artifacts"
            }[faction],
            "sample_texts": role_samples[role]
        })

BIBLE = {
    "schema_version": "1.0",
    "model": "openbmb/VoxCPM2",
    "device": "mps",
    "language": "ru-RU",
    "voices": VOICES,
    "test_package": {
        "total_voices": len(VOICES),
        "anchors_per_voice": 3,
        "lines_per_voice_test": 5,
        "estimated_test_duration_minutes": 35
    }
}

os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(BIBLE, f, ensure_ascii=False, indent=2)
print(f"Wrote {OUT} with {len(VOICES)} voices")
print(f"  - 4 EVA: " + ", ".join(v['id'] for v in VOICES if v['role']=='EVA'))
print(f"  - 16 Unit voices across 4 factions")
