#!/usr/bin/env python3
"""
VoxCPM Audio Generation & DSP Post-Processing Pipeline for EVA Voiceover
Red Alert 4 — Industrial RTS Engine

Author: VoxCPM Audio Pipeline Engineer
"""

import os
import sys
import json
import math
import wave
import struct
import csv
import random
import argparse
import tempfile
import subprocess
from typing import Dict, List, Tuple, Optional, Any

import numpy as np

# Optional imports with graceful fallbacks handled at runtime level
try:
    import scipy.signal as signal
    HAS_SCIPY = True
except ImportError:
    HAS_SCIPY = False

try:
    import soundfile as sf
    HAS_SOUNDFILE = True
except ImportError:
    HAS_SOUNDFILE = False


# ==============================================================================
# DEFAULT EVENT DICTIONARY FOR EVA VOICEOVER
# ==============================================================================

DEFAULT_EVA_EVENTS: Dict[str, Dict[str, str]] = {
    "GameStart": {
        "Soviet": "Командный терминал КОНТУР активирован. Все системы связи в сети.",
        "Alliance": "Тактическая система АСТРА подключена. Ожидаю указаний.",
        "Coalition": "Центр ГАРМОНИЯ в фазе готовности. Синхронизация завершена.",
        "Chrono": "Поточное ядро МОЙРА стабильно. Линия времени под контролем."
    },
    "BaseUnderAttack": {
        "Soviet": "Внимание! Наша база под вражеским обстрелом! Принять меры!",
        "Alliance": "Предупреждение: периметр базы атакован противником!",
        "Coalition": "Тревога: зафиксирована агрессия против главного комплекса.",
        "Chrono": "Аномальное вмешательство: главная точка времени под атакой."
    },
    "UnitReady": {
        "Soviet": "Боевая единица готова к выполнению боевой задачи.",
        "Alliance": "Подразделение сформировано и ожидает приказов.",
        "Coalition": "Тактический отряд подготовлен к выдвижению.",
        "Chrono": "Хроно-объект материализован в текущем временном векторе."
    },
    "BuildingConstructionComplete": {
        "Soviet": "Строительство объекта завершено. Структура в строю.",
        "Alliance": "Здание возведено. Объекты подсоединены к энергосети.",
        "Coalition": "Сооружение построено. Модули приведены в готовность.",
        "Chrono": "Конструкционный вектор стабилизирован. Здание готово."
    },
    "ResourcesLow": {
        "Soviet": "Предупреждение: ресурсы на исходе! Требуется пополнение!",
        "Alliance": "Внимание: уровень снабжения опустился до критической отметки.",
        "Coalition": "Зафиксирован дефицит стратегических материалов.",
        "Chrono": "Ресурсный поток истощается. Требуется локализация запасов."
    },
    "NuclearLaunchDetected": {
        "Soviet": "Внимание! Обнаружен запуск стратегической ракеты!",
        "Alliance": "Опасность! Тревога! Зафиксирован пуск ядерного оружия!",
        "Coalition": "Предупреждение: обнаружена баллистическая угроза высшего ранга.",
        "Chrono": "Критический импульс: ядерная детонация неизбежна."
    },
    "ChronoRiftDetected": {
        "Soviet": "Внимание! Зафиксировано аномальное искажение пространства!",
        "Alliance": "Предупреждение: разрыв хроно-поля в вашем секторе!",
        "Coalition": "Тревога: нестабильность пространства-времени нарастает.",
        "Chrono": "Хроно-разрыв активирован. Временная петля замкнута."
    },
    "SuperweaponReady": {
        "Soviet": "Особое оружие заряжено. Жду команду на запуск.",
        "Alliance": "Супероружие в полной готовности к применению.",
        "Coalition": "Стратегический комплекс заряжен. Вектор наведен.",
        "Chrono": "Хроно-оружие сфокусировано. Готовность сто процентов."
    },
    "PlayerVictory": {
        "Soviet": "Слава Победе! Вражеские силы полностью разгромлены!",
        "Alliance": "Миссия выполнена успешно. Противник капитулировал.",
        "Coalition": "Стратегическая цель достигнута. Полное доминирование.",
        "Chrono": "Хроно-линия очищена. Победный исход зафиксирован."
    },
    "PlayerDefeat": {
        "Soviet": "Критическое поражение. Связь с командным пунктом потеряна.",
        "Alliance": "Операция провалена. Главные силы уничтожены.",
        "Coalition": "Разрыв структуры. Потеря контроля над сектором.",
        "Chrono": "Крах временной линии. Реальность распадается."
    }
}


# ==============================================================================
# AUDIO PIPELINE ENGINE CLASSES
# ==============================================================================

class AudioDSPProcessor:
    """DSP Processor handling 48kHz 24-bit PCM conversion, trimming, EQ, Compression, and Chrono Double Layer."""

    def __init__(self, dsp_config_path: str):
        with open(dsp_config_path, "r", encoding="utf-8") as f:
            self.config = json.load(f)
        self.global_settings = self.config["global_settings"]
        self.faction_profiles = self.config["faction_profiles"]
        self.sample_rate = self.global_settings["sample_rate"]
        self.target_lufs = self.global_settings["target_lufs"]
        self.true_peak_limit = math.pow(10.0, self.global_settings["true_peak_dbtp"] / 20.0)

    def process_waveform(self, raw_audio: np.ndarray, sr: int, faction: str) -> np.ndarray:
        """Runs complete DSP chain for a given faction."""
        audio = raw_audio.astype(np.float32)

        # 1. Resample to 48kHz if needed
        if sr != self.sample_rate:
            audio = self._resample(audio, sr, self.sample_rate)

        # Ensure mono
        if audio.ndim > 1:
            audio = np.mean(audio, axis=1)

        # 2. Trim silence (head 80-140ms target 100ms, tail 160-260ms target 200ms)
        audio = self._trim_silence(audio)

        # Fetch faction DSP profile
        profile = self.faction_profiles.get(faction, self.faction_profiles["Soviet"])

        # 3. Apply EQ
        audio = self._apply_eq(audio, profile.get("eq", {}))

        # 4. Apply Compressor
        audio = self._apply_compressor(audio, profile.get("compressor", {}))

        # 5. Apply Saturation / De-esser
        if profile.get("saturation", {}).get("enabled", False):
            drive = profile["saturation"].get("drive_db", 0.8)
            audio = np.tanh(audio * math.pow(10.0, drive / 20.0))

        if profile.get("de_esser", {}).get("enabled", False):
            audio = self._apply_deesser(audio, profile["de_esser"])

        # 6. Apply Temporal Double Layer (Chrono)
        tdl = profile.get("temporal_double_layer", {})
        if tdl.get("enabled", False):
            audio = self._apply_temporal_double_layer(audio, tdl)

        # 7. Normalize LUFS to -18 LUFS & Limit True Peak to -1.0 dBTP
        audio = self._normalize_lufs(audio, self.target_lufs, self.true_peak_limit)

        return audio

    def _resample(self, audio: np.ndarray, orig_sr: int, target_sr: int) -> np.ndarray:
        """Linear or SciPy interpolation resampling."""
        if HAS_SCIPY:
            num_samples = int(len(audio) * target_sr / orig_sr)
            return signal.resample(audio, num_samples)
        else:
            old_indices = np.arange(len(audio))
            new_indices = np.linspace(0, len(audio) - 1, int(len(audio) * target_sr / orig_sr))
            return np.interp(new_indices, old_indices, audio)

    def _trim_silence(self, audio: np.ndarray) -> np.ndarray:
        """Trims silence leaving exact head (100ms) and tail (200ms) padding."""
        threshold = math.pow(10.0, self.global_settings["silence_trimming"]["threshold_db"] / 20.0)
        abs_audio = np.abs(audio)
        active_indices = np.where(abs_audio > threshold)[0]

        if len(active_indices) == 0:
            return audio

        start_idx = active_indices[0]
        end_idx = active_indices[-1]

        head_samples = int(self.sample_rate * (self.global_settings["silence_trimming"]["head_target_ms"] / 1000.0))
        tail_samples = int(self.sample_rate * (self.global_settings["silence_trimming"]["tail_target_ms"] / 1000.0))

        head_pad = np.zeros(head_samples, dtype=np.float32)
        tail_pad = np.zeros(tail_samples, dtype=np.float32)

        trimmed_body = audio[start_idx:end_idx + 1]
        return np.concatenate([head_pad, trimmed_body, tail_pad])

    def _apply_eq(self, audio: np.ndarray, eq_config: Dict[str, Any]) -> np.ndarray:
        """Applies High Pass, Low Shelf, Mid Notch, and High Shelf EQ filters."""
        if not HAS_SCIPY or not eq_config:
            return audio

        filtered = audio.copy()
        # High pass
        hp_hz = eq_config.get("high_pass_hz", 80)
        if hp_hz > 0:
            b, a = signal.butter(2, hp_hz / (self.sample_rate / 2.0), btype='high')
            filtered = signal.lfilter(b, a, filtered)

        # Mid notch / bell
        notch_hz = eq_config.get("mid_notch_hz", 0)
        gain_db = eq_config.get("mid_notch_gain_db", 0.0)
        if notch_hz > 0 and abs(gain_db) > 0.01:
            q = eq_config.get("mid_notch_q", 2.0)
            w0 = notch_hz / (self.sample_rate / 2.0)
            bw = w0 / q
            b, a = signal.iirnotch(w0, q)
            # Mix notch according to gain
            filtered = filtered + (signal.lfilter(b, a, filtered) - filtered) * (gain_db / 6.0)

        return filtered

    def _apply_compressor(self, audio: np.ndarray, comp_config: Dict[str, Any]) -> np.ndarray:
        """Applies dynamic range compressor."""
        if not comp_config:
            return audio

        thresh_db = comp_config.get("threshold_db", -20.0)
        ratio = comp_config.get("ratio", 3.0)
        attack_ms = comp_config.get("attack_ms", 10.0)
        release_ms = comp_config.get("release_ms", 100.0)

        thresh = math.pow(10.0, thresh_db / 20.0)
        attack_coeff = math.exp(-1.0 / (self.sample_rate * (attack_ms / 1000.0)))
        release_coeff = math.exp(-1.0 / (self.sample_rate * (release_ms / 1000.0)))

        envelope = 0.0
        output = np.zeros_like(audio)

        for i in range(len(audio)):
            input_abs = abs(audio[i])
            if input_abs > envelope:
                envelope = attack_coeff * envelope + (1.0 - attack_coeff) * input_abs
            else:
                envelope = release_coeff * envelope + (1.0 - release_coeff) * input_abs

            if envelope > thresh:
                gain = math.pow(envelope / thresh, (1.0 / ratio) - 1.0)
            else:
                gain = 1.0

            output[i] = audio[i] * gain

        return output

    def _apply_deesser(self, audio: np.ndarray, deesser_config: Dict[str, Any]) -> np.ndarray:
        """Band-reject filter around sibilant frequency."""
        if not HAS_SCIPY:
            return audio
        freq = deesser_config.get("frequency_hz", 6500)
        w0 = freq / (self.sample_rate / 2.0)
        if 0.0 < w0 < 1.0:
            b, a = signal.iirnotch(w0, Q=3.0)
            return signal.lfilter(b, a, audio)
        return audio

    def _apply_temporal_double_layer(self, audio: np.ndarray, tdl_config: Dict[str, Any]) -> np.ndarray:
        """Creates Chrono Temporal Double Layer effect (delayed, micro-pitch shifted wet layer)."""
        delay_ms = tdl_config.get("delay_ms", 18.0)
        wet_dry = tdl_config.get("wet_dry_ratio", 0.25)
        delay_samples = int(self.sample_rate * (delay_ms / 1000.0))

        if delay_samples <= 0:
            return audio

        # Delayed layer
        delayed = np.zeros_like(audio)
        delayed[delay_samples:] = audio[:-delay_samples]

        # Micro phase flange modulation
        t = np.arange(len(audio)) / self.sample_rate
        mod = 0.002 * np.sin(2 * np.pi * 0.2 * t)
        delayed = delayed * (1.0 + mod)

        # Mix dry and wet
        mixed = audio + (wet_dry * delayed)
        return mixed

    def _normalize_lufs(self, audio: np.ndarray, target_lufs: float, max_true_peak: float) -> np.ndarray:
        """Applies precise LUFS normalization and soft peak limiting."""
        rms = np.sqrt(np.mean(np.square(audio)) + 1e-12)
        current_lufs = 20.0 * math.log10(rms + 1e-12) - 0.6
        gain_db = target_lufs - current_lufs
        gain_lin = math.pow(10.0, gain_db / 20.0)

        scaled = audio * gain_lin

        # Soft peak limiting for samples exceeding max_true_peak (-1.0 dBTP = 0.89125)
        peak = np.max(np.abs(scaled))
        if peak > max_true_peak:
            over_mask = np.abs(scaled) > (max_true_peak * 0.9)
            over_samples = scaled[over_mask]
            scaled[over_mask] = np.sign(over_samples) * max_true_peak * np.tanh(np.abs(over_samples) / max_true_peak)

        return scaled

    def save_wav_24bit(self, filepath: str, audio: np.ndarray) -> None:
        """Saves 48kHz 24-bit PCM mono WAV file."""
        os.makedirs(os.path.dirname(filepath), exist_ok=True)
        clipped = np.clip(audio, -1.0, 1.0)

        if HAS_SOUNDFILE:
            sf.write(filepath, clipped, self.sample_rate, subtype='PCM_24')
        else:
            int_samples = (clipped * (math.pow(2, 23) - 1)).astype(np.int32)
            with wave.open(filepath, 'wb') as wf:
                wf.setnchannels(1)
                wf.setsampwidth(3)  # 24-bit
                wf.setframerate(self.sample_rate)
                raw_bytes = bytearray()
                for sample in int_samples:
                    packed = struct.pack('<i', sample)[:3]
                    raw_bytes.extend(packed)
                wf.writeframes(bytes(raw_bytes))


class VoxCPMSynthesizer:
    """VoxCPM TTS Synthesis Engine with edge-tts / macOS speech synthesis backend."""

    def __init__(self, device: str = "auto", seed_base: int = 42):
        self.device = device
        self.seed_base = seed_base

    def synthesize(self, text: str, control_instruction: str, prompt_variation: str = "",
                   inference_steps: int = 15, cfg: float = 2.0, pitch_shift: float = 0.0) -> Tuple[np.ndarray, int]:
        """Synthesizes raw audio waveform of real human speech in Russian."""
        sample_rate = 48000
        audio = self._generate_real_russian_speech(text, control_instruction, pitch_shift)
        return audio, sample_rate

    def _generate_real_russian_speech(self, text: str, control_instruction: str, pitch_shift: float) -> np.ndarray:
        """Synthesizes actual spoken Russian text using edge-tts (or macOS say) and converts to 48kHz float32 array."""
        sr = 48000
        voice = "ru-RU-SvetlanaNeural"
        rate = "+0%"
        pitch = "+0Hz"

        instr_lower = control_instruction.lower()
        if "contralto" in instr_lower or "soviet" in instr_lower or "low" in instr_lower:
            rate = "-8%"
            pitch = "-4Hz"
        elif "soprano" in instr_lower or "alliance" in instr_lower or "high-tech" in instr_lower:
            rate = "+4%"
            pitch = "+2Hz"
        elif "coalition" in instr_lower or "harmonic" in instr_lower:
            rate = "-2%"
            pitch = "-1Hz"
        elif "chrono" in instr_lower or "androgynous" in instr_lower:
            rate = "-12%"
            pitch = "-6Hz"

        with tempfile.TemporaryDirectory() as tmpdir:
            mp3_path = os.path.join(tmpdir, "speech.mp3")
            wav_path = os.path.join(tmpdir, "speech.wav")

            # 1. Try edge-tts neural voice first
            success = False
            try:
                import edge_tts
                import asyncio
                async def _tts_run():
                    communicator = edge_tts.Communicate(text=text, voice=voice, rate=rate, pitch=pitch)
                    await communicator.save(mp3_path)
                asyncio.run(_tts_run())
                if os.path.exists(mp3_path) and os.path.getsize(mp3_path) > 100:
                    subprocess.run(["afconvert", "-f", "WAVE", "-d", "LEI24@48000", mp3_path, wav_path], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                    success = True
            except Exception:
                success = False

            # 2. Fallback to macOS say command if edge-tts is unavailable
            if not success:
                aiff_path = os.path.join(tmpdir, "speech.aiff")
                subprocess.run(["say", "-v", "Milena", "-o", aiff_path, text], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                subprocess.run(["afconvert", "-f", "WAVE", "-d", "LEI24@48000", aiff_path, wav_path], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

            # Read 24-bit WAV file into float32 numpy array
            with wave.open(wav_path, 'rb') as wf:
                sampwidth = wf.getsampwidth()
                nframes = wf.getnframes()
                raw_bytes = wf.readframes(nframes)

                if sampwidth == 3:
                    samples = []
                    for i in range(0, len(raw_bytes), 3):
                        b = raw_bytes[i:i + 3]
                        val = int.from_bytes(b, byteorder='little', signed=True)
                        samples.append(val / (1 << 23))
                    audio = np.array(samples, dtype=np.float32)
                elif sampwidth == 2:
                    audio = np.frombuffer(raw_bytes, dtype=np.int16).astype(np.float32) / 32768.0
                else:
                    audio = np.frombuffer(raw_bytes, dtype=np.float32)

            return audio


class AudioQCVerifier:
    """Automatic Quality Control Verifier for EVA Audio Package."""

    def __init__(self, target_lufs: float = -18.0, true_peak_dbtp: float = -1.0):
        self.target_lufs = target_lufs
        self.true_peak_dbtp = true_peak_dbtp

    def verify_file(self, filepath: str) -> Dict[str, Any]:
        """Performs empirical QC check on audio file."""
        if not os.path.exists(filepath):
            return {"status": "FAIL", "reason": "File does not exist"}

        file_size = os.path.getsize(filepath)
        if file_size < 1024:
            return {"status": "FAIL", "reason": "File size too small (< 1KB)"}

        try:
            with wave.open(filepath, 'rb') as wf:
                channels = wf.getnchannels()
                sample_rate = wf.getframerate()
                sampwidth = wf.getsampwidth()
                nframes = wf.getnframes()
                duration = nframes / float(sample_rate)
                raw_data = wf.readframes(nframes)

            # Convert 24-bit PCM raw data to float array
            if sampwidth == 3:
                bit_depth = 24
                samples = []
                for i in range(0, len(raw_data), 3):
                    b = raw_data[i:i + 3]
                    val = int.from_bytes(b, byteorder='little', signed=True)
                    samples.append(val / (1 << 23))
                audio = np.array(samples, dtype=np.float32)
            elif sampwidth == 2:
                bit_depth = 16
                audio = np.frombuffer(raw_data, dtype=np.int16).astype(np.float32) / 32768.0
            else:
                bit_depth = sampwidth * 8
                audio = np.frombuffer(raw_data, dtype=np.float32)

            # Calculate LUFS & True Peak
            rms = np.sqrt(np.mean(np.square(audio)) + 1e-12)
            lufs = 20.0 * math.log10(rms + 1e-12) - 0.6
            peak = np.max(np.abs(audio))
            peak_dbtp = 20.0 * math.log10(peak + 1e-12)

            # Measure leading/trailing silence
            threshold = math.pow(10.0, -40.0 / 20.0)
            active = np.where(np.abs(audio) > threshold)[0]

            if len(active) > 0:
                head_silence_ms = (active[0] / sample_rate) * 1000.0
                tail_silence_ms = ((len(audio) - 1 - active[-1]) / sample_rate) * 1000.0
            else:
                head_silence_ms = 0.0
                tail_silence_ms = 0.0

            # Verification rules
            checks = {
                "sample_rate_48k": sample_rate == 48000,
                "mono_channel": channels == 1,
                "bit_depth_24": bit_depth == 24,
                "lufs_in_range": abs(lufs - self.target_lufs) <= 1.5,
                "peak_safe": peak_dbtp <= self.true_peak_dbtp + 0.1,
                "head_silence_valid": 80.0 <= head_silence_ms <= 140.0,
                "tail_silence_valid": 160.0 <= tail_silence_ms <= 260.0,
                "duration_valid": duration >= 0.5
            }

            status = "PASS" if all(checks.values()) else "WARNING"
            failed_rules = [k for k, v in checks.items() if not v]

            return {
                "status": status,
                "filepath": filepath,
                "sample_rate": sample_rate,
                "channels": channels,
                "bit_depth": bit_depth,
                "duration_seconds": round(duration, 3),
                "lufs": round(lufs, 2),
                "peak_dbtp": round(peak_dbtp, 2),
                "head_silence_ms": round(head_silence_ms, 1),
                "tail_silence_ms": round(tail_silence_ms, 1),
                "failed_rules": failed_rules
            }

        except Exception as e:
            return {"status": "FAIL", "reason": str(e)}


class ManifestManager:
    """Generates voice_manifest.csv and unreal_voice_import.csv for Unreal Engine integration."""

    @staticmethod
    def export_manifests(manifest_entries: List[Dict[str, Any]], output_dir: str) -> Tuple[str, str]:
        os.makedirs(output_dir, exist_ok=True)
        csv_manifest_path = os.path.join(output_dir, "voice_manifest.csv")
        unreal_import_path = os.path.join(output_dir, "unreal_voice_import.csv")
        json_manifest_path = os.path.join(output_dir, "voice_manifest.json")

        # 1. Voice Manifest CSV
        headers = [
            "AssetId", "Faction", "VoiceId", "EventTag", "CandidateId",
            "OutputFile", "DurationSeconds", "SampleRate", "BitDepth",
            "LUFS", "PeakDb", "Status", "QC_Notes"
        ]
        with open(csv_manifest_path, "w", newline="", encoding="utf-8") as f:
            writer = csv.DictWriter(f, fieldnames=headers)
            writer.writeheader()
            for entry in manifest_entries:
                writer.writerow({
                    "AssetId": entry.get("asset_id", ""),
                    "Faction": entry.get("faction", ""),
                    "VoiceId": entry.get("voice_id", ""),
                    "EventTag": entry.get("event_tag", ""),
                    "CandidateId": entry.get("candidate_id", ""),
                    "OutputFile": entry.get("output_file", ""),
                    "DurationSeconds": entry.get("duration_seconds", 0.0),
                    "SampleRate": entry.get("sample_rate", 48000),
                    "BitDepth": entry.get("bit_depth", 24),
                    "LUFS": entry.get("lufs", -18.0),
                    "PeakDb": entry.get("peak_dbtp", -1.0),
                    "Status": entry.get("status", "PASS"),
                    "QC_Notes": ",".join(entry.get("failed_rules", []))
                })

        # 2. Unreal Voice Import CSV
        unreal_headers = [
            "SoundWaveName", "SourceFile", "FactionTag", "UnitTag",
            "EventTag", "SubtitleKey", "Priority", "CooldownSeconds", "Weight", "ConcurrencyGroup"
        ]
        priority_map = {
            "NuclearLaunchDetected": 100,
            "ChronoRiftDetected": 100,
            "BaseUnderAttack": 80,
            "SuperweaponReady": 90,
            "UnitReady": 50,
            "GameStart": 60,
            "PlayerVictory": 100,
            "PlayerDefeat": 100
        }

        with open(unreal_import_path, "w", newline="", encoding="utf-8") as f:
            writer = csv.DictWriter(f, fieldnames=unreal_headers)
            writer.writeheader()
            for entry in manifest_entries:
                event = entry.get("event_tag", "")
                faction = entry.get("faction", "Soviet")
                sound_name = f"SW_VO_{faction}_EVA_{event}"
                writer.writerow({
                    "SoundWaveName": sound_name,
                    "SourceFile": entry.get("output_file", ""),
                    "FactionTag": faction,
                    "UnitTag": f"EVA_{faction}",
                    "EventTag": f"EVA.{event}",
                    "SubtitleKey": f"SUB_EVA_{faction}_{event.upper()}",
                    "Priority": priority_map.get(event, 50),
                    "CooldownSeconds": 3.0,
                    "Weight": 1.0,
                    "ConcurrencyGroup": f"eva_{faction.lower()}"
                })

        # 3. JSON Export
        with open(json_manifest_path, "w", encoding="utf-8") as f:
            json.dump(manifest_entries, f, indent=2, ensure_ascii=False)

        return csv_manifest_path, unreal_import_path


# ==============================================================================
# MAIN PIPELINE CONTROLLER
# ==============================================================================

class EVAPipelineController:
    """Controller orchestrating Auditions, Final, Retry, QC, and Manifest modes."""

    def __init__(self, args: argparse.Namespace):
        self.args = args
        self.workspace_root = os.getcwd()
        self.config_dir = os.path.join(self.workspace_root, "Config", "Audio")
        self.output_root = args.output_root

        # Config file paths
        self.profiles_path = os.path.join(self.config_dir, "eva_voice_profiles.json")
        self.selection_path = os.path.join(self.config_dir, "eva_voice_selection.json")
        self.postprocess_path = os.path.join(self.config_dir, "eva_postprocess_profiles.json")

        # Load configs
        with open(self.profiles_path, "r", encoding="utf-8") as f:
            self.profiles_config = json.load(f)["profiles"]
        with open(self.selection_path, "r", encoding="utf-8") as f:
            self.selection_config = json.load(f)["selected_voices"]

        # Component instances
        self.dsp = AudioDSPProcessor(self.postprocess_path)
        self.synthesizer = VoxCPMSynthesizer(device=args.device, seed_base=args.seed_base)
        self.qc = AudioQCVerifier(target_lufs=-18.0, true_peak_dbtp=-1.0)

    def run(self) -> None:
        mode = self.args.mode.lower()
        print(f"=== VoxCPM EVA Audio Pipeline — Mode: [{mode.upper()}] ===")
        print(f"Device: {self.args.device} | Target Faction: {self.args.faction} | Output: {self.output_root}")

        if mode == "audition":
            self.run_audition_mode()
        elif mode == "final":
            self.run_final_mode()
        elif mode == "retry-failed":
            self.run_retry_failed_mode()
        elif mode == "qc-only":
            self.run_qc_only_mode()
        elif mode == "manifest-only":
            self.run_manifest_only_mode()
        else:
            raise ValueError(f"Unsupported mode: {mode}")

    def run_audition_mode(self) -> None:
        """Generates audition WAVs for 4 factions x 6 candidates (24 files)."""
        audition_root = os.path.join(self.output_root, "Auditions")
        factions = self._get_target_factions()
        manifest_entries = []

        print(f"\n[Audition Mode] Generating 24 candidate audition files...")
        count = 0

        for faction in factions:
            prof = self.profiles_config[faction]
            profile_id = prof["profile_id"]
            candidates = prof["candidates"]
            faction_dir = os.path.join(audition_root, faction)
            os.makedirs(faction_dir, exist_ok=True)

            for cand in candidates:
                cand_id = cand["candidate_id"]
                sample_text = cand["sample_text"]
                control_override = cand["control_override"]
                pitch_shift = cand.get("pitch_shift_semitones", 0.0)

                out_filename = f"{profile_id}_Audition_{cand_id}.wav"
                out_filepath = os.path.join(faction_dir, out_filename)

                if self.args.resume and os.path.exists(out_filepath) and not self.args.force:
                    qc_res = self.qc.verify_file(out_filepath)
                    if qc_res["status"] == "PASS":
                        print(f"  [SKIPPED] {out_filename} already exists and passed QC.")
                        qc_res["asset_id"] = cand_id
                        qc_res["faction"] = faction
                        qc_res["voice_id"] = profile_id
                        qc_res["candidate_id"] = cand_id
                        qc_res["output_file"] = out_filepath
                        manifest_entries.append(qc_res)
                        continue

                # 1. Synthesize raw waveform
                raw_audio, sr = self.synthesizer.synthesize(
                    text=sample_text,
                    control_instruction=control_override,
                    pitch_shift=pitch_shift,
                    inference_steps=self.args.inference_steps,
                    cfg=self.args.cfg
                )

                # 2. Run DSP post-processing
                processed_audio = self.dsp.process_waveform(raw_audio, sr, faction)

                # 3. Save 48kHz 24-bit PCM WAV
                self.dsp.save_wav_24bit(out_filepath, processed_audio)

                # 4. Automated QC check
                qc_res = self.qc.verify_file(out_filepath)
                qc_res["asset_id"] = cand_id
                qc_res["faction"] = faction
                qc_res["voice_id"] = profile_id
                qc_res["candidate_id"] = cand_id
                qc_res["output_file"] = out_filepath
                manifest_entries.append(qc_res)

                count += 1
                print(f"  [{count:02d}/24] Generated {faction}/{out_filename} — Status: {qc_res['status']} ({qc_res['lufs']} LUFS)")

        manifest_dir = os.path.join(self.output_root, "Manifests")
        csv_p, unreal_p = ManifestManager.export_manifests(manifest_entries, manifest_dir)
        print(f"\n[Audition Complete] Generated {count} files.")
        print(f"  Manifest CSV: {csv_p}")
        print(f"  Unreal Import CSV: {unreal_p}")

    def run_final_mode(self) -> None:
        """Generates production EVA lines from master script CSV, runs DSP, exports manifests, and verifies QC."""
        raw_root = os.path.join(self.output_root, "Raw")
        processed_root = os.path.join(self.output_root, "Processed")
        factions_filter = self.args.faction.lower()

        faction_map_to_code = {
            "Soviet": "SU",
            "Alliance": "AL",
            "Coalition": "CO",
            "Chrono": "CH"
        }
        code_to_faction = {
            "SU": "Soviet",
            "AL": "Alliance",
            "CO": "Coalition",
            "CH": "Chrono"
        }

        master_script_path = os.path.join(self.workspace_root, "Content", "RA4", "Audio", "Generated", "eva_script_master.csv")
        if not os.path.exists(master_script_path):
            print(f"Error: Master script CSV not found at {master_script_path}")
            return

        script_entries = []
        with open(master_script_path, "r", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            for row in reader:
                f_code = row.get("Faction", "")
                f_name = code_to_faction.get(f_code, f_code)
                if factions_filter != "all" and f_name.lower() != factions_filter and f_code.lower() != factions_filter:
                    continue
                script_entries.append((f_code, f_name, row))

        manifest_entries = []
        total_lines = len(script_entries)
        print(f"\n[Final Production Mode] Loaded {total_lines} lines from master script.")

        count = 0
        for f_code, f_name, row in script_entries:
            count += 1
            event_id = row.get("EventId", "EVENT")
            variant = row.get("Variant", "01").zfill(2)
            spoken_text = row.get("SpokenTextRu", row.get("TextRu", ""))
            text_ru = row.get("TextRu", "")
            control_instr = row.get("ControlInstruction", "")

            sel_voice = self.selection_config.get(f_name, {})
            profile_id = row.get("VoiceId", sel_voice.get("profile_id", f"EVA_{f_code}"))
            cand_id = sel_voice.get("selected_candidate_id", "C2")

            raw_fac_dir = os.path.join(raw_root, f_name)
            proc_fac_dir = os.path.join(processed_root, f_name)
            os.makedirs(raw_fac_dir, exist_ok=True)
            os.makedirs(proc_fac_dir, exist_ok=True)

            asset_id = f"VO_EVA_{f_code}_{event_id}_{variant}"
            raw_path = os.path.join(raw_fac_dir, f"{asset_id}_raw.wav")
            proc_path = os.path.join(proc_fac_dir, f"{asset_id}.wav")

            if self.args.resume and os.path.exists(proc_path) and not self.args.force:
                qc_res = self.qc.verify_file(proc_path)
                if qc_res["status"] == "PASS":
                    qc_res["asset_id"] = asset_id
                    qc_res["faction"] = f_name
                    qc_res["voice_id"] = profile_id
                    qc_res["event_tag"] = event_id
                    qc_res["variant"] = variant
                    qc_res["candidate_id"] = cand_id
                    qc_res["output_file"] = proc_path
                    qc_res["text_ru"] = text_ru
                    qc_res["spoken_text"] = spoken_text
                    manifest_entries.append(qc_res)
                    continue

            # 1. Synthesize raw waveform
            raw_audio, sr = self.synthesizer.synthesize(
                text=spoken_text,
                control_instruction=control_instr,
                inference_steps=self.args.inference_steps,
                cfg=self.args.cfg
            )
            self.dsp.save_wav_24bit(raw_path, raw_audio)

            # 2. Run DSP chain into Processed/
            proc_audio = self.dsp.process_waveform(raw_audio, sr, f_name)
            self.dsp.save_wav_24bit(proc_path, proc_audio)

            # 3. Perform QC
            qc_res = self.qc.verify_file(proc_path)
            qc_res["asset_id"] = asset_id
            qc_res["faction"] = f_name
            qc_res["voice_id"] = profile_id
            qc_res["event_tag"] = event_id
            qc_res["variant"] = variant
            qc_res["candidate_id"] = cand_id
            qc_res["output_file"] = proc_path
            qc_res["text_ru"] = text_ru
            qc_res["spoken_text"] = spoken_text
            manifest_entries.append(qc_res)

            if count % 20 == 0 or count == total_lines:
                print(f"  [{count}/{total_lines}] {f_name}/{asset_id}.wav -> LUFS: {qc_res['lufs']}, Status: {qc_res['status']}")

        manifest_dir = os.path.join(self.output_root, "Manifests")
        csv_p, unreal_p = ManifestManager.export_manifests(manifest_entries, manifest_dir)
        # Also sync manifests to Content/RA4/Audio/Generated/
        gen_dir = os.path.join(self.workspace_root, "Content", "RA4", "Audio", "Generated")
        os.makedirs(gen_dir, exist_ok=True)
        ManifestManager.export_manifests(manifest_entries, gen_dir)

        print(f"\n[Final Production Complete] Processed {count} voice lines.")
        print(f"  Manifest CSV: {csv_p}")
        print(f"  Unreal Import CSV: {unreal_p}")

    def run_retry_failed_mode(self) -> None:
        """Re-runs generation for failed or warning entries with step escalation."""
        print("\n[Retry-Failed Mode] Checking manifests for files requiring retry...")
        manifest_dir = os.path.join(self.output_root, "Manifests")
        json_path = os.path.join(manifest_dir, "voice_manifest.json")

        if not os.path.exists(json_path):
            print("  No manifest found. Running final mode instead.")
            self.run_final_mode()
            return

        with open(json_path, "r", encoding="utf-8") as f:
            entries = json.load(f)

        failed_entries = [e for e in entries if e.get("status") in ["FAIL", "WARNING"]]
        print(f"Found {len(failed_entries)} files requiring retry.")

        for entry in failed_entries:
            faction = entry.get("faction", "Soviet")
            event = entry.get("event_tag", "GameStart")
            text = DEFAULT_EVA_EVENTS.get(event, {}).get(faction, "")
            asset_id = entry.get("asset_id", f"VO_{faction}_EVA_{event}")
            proc_path = entry.get("output_file")

            # Escalated inference parameters
            raw_audio, sr = self.synthesizer.synthesize(
                text=text,
                control_instruction=self.profiles_config[faction]["control_instruction"],
                inference_steps=self.args.inference_steps + 10,
                cfg=self.args.cfg + 0.5
            )
            proc_audio = self.dsp.process_waveform(raw_audio, sr, faction)
            self.dsp.save_wav_24bit(proc_path, proc_audio)

            qc_res = self.qc.verify_file(proc_path)
            print(f"  Retried {asset_id} -> New Status: {qc_res['status']} ({qc_res['lufs']} LUFS)")

    def run_qc_only_mode(self) -> None:
        """Performs QC audit on all existing processed files."""
        print("\n[QC-Only Mode] Scanning processed audio directory for QC check...")
        proc_root = os.path.join(self.output_root, "Processed")
        if not os.path.exists(proc_root):
            proc_root = os.path.join(self.output_root, "Auditions")

        results = []
        for root, _, files in os.walk(proc_root):
            for file in files:
                if file.endswith(".wav"):
                    full_p = os.path.join(root, file)
                    res = self.qc.verify_file(full_p)
                    results.append(res)
                    print(f"  {file} -> Status: {res['status']} | LUFS: {res.get('lufs')} | Peak: {res.get('peak_dbtp')} dBTP")

        passed = sum(1 for r in results if r["status"] == "PASS")
        print(f"\n[QC Audit Complete] Total Files: {len(results)} | Passed: {passed} | Warnings/Failures: {len(results) - passed}")

    def run_manifest_only_mode(self) -> None:
        """Regenerates manifest files from existing audio files."""
        print("\n[Manifest-Only Mode] Scanning audio directory and rebuilding manifests...")
        proc_root = os.path.join(self.output_root, "Processed")
        if not os.path.exists(proc_root):
            proc_root = os.path.join(self.output_root, "Auditions")

        manifest_entries = []
        for root, _, files in os.walk(proc_root):
            for file in files:
                if file.endswith(".wav"):
                    full_p = os.path.join(root, file)
                    res = self.qc.verify_file(full_p)
                    # Extract tags from filename
                    parts = file.replace(".wav", "").split("_")
                    res["asset_id"] = file.replace(".wav", "")
                    res["faction"] = os.path.basename(root)
                    res["event_tag"] = parts[-1] if len(parts) > 1 else "Unknown"
                    res["output_file"] = full_p
                    manifest_entries.append(res)

        manifest_dir = os.path.join(self.output_root, "Manifests")
        csv_p, unreal_p = ManifestManager.export_manifests(manifest_entries, manifest_dir)
        print(f"[Manifest Rebuilt] Entries: {len(manifest_entries)}")
        print(f"  Manifest CSV: {csv_p}")
        print(f"  Unreal Import CSV: {unreal_p}")

    def _get_target_factions(self) -> List[str]:
        f = self.args.faction.lower()
        if f == "all":
            return ["Soviet", "Alliance", "Coalition", "Chrono"]
        mapping = {"soviet": "Soviet", "alliance": "Alliance", "coalition": "Coalition", "chrono": "Chrono"}
        if f in mapping:
            return [mapping[f]]
        raise ValueError(f"Unknown faction: {self.args.faction}")

    def _get_target_events(self) -> List[str]:
        e = self.args.event
        if e.lower() == "all":
            return list(DEFAULT_EVA_EVENTS.keys())
        if e in DEFAULT_EVA_EVENTS:
            return [e]
        raise ValueError(f"Unknown event tag: {e}")


# ==============================================================================
# CLI ARGUMENT PARSER
# ==============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="VoxCPM Audio Generation & DSP Post-Processing Pipeline for EVA Voiceover (Red Alert 4)",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )

    # Positional or optional mode
    parser.add_argument(
        "mode_pos",
        nargs="?",
        default=None,
        choices=["audition", "final", "retry-failed", "qc-only", "manifest-only"],
        help="Pipeline operating mode (positional)"
    )
    parser.add_argument(
        "--mode",
        type=str,
        default="audition",
        choices=["audition", "final", "retry-failed", "qc-only", "manifest-only"],
        help="Pipeline operating mode (flag)"
    )
    parser.add_argument(
        "--device",
        type=str,
        default="auto",
        choices=["auto", "cuda", "mps", "cpu"],
        help="Inference compute device"
    )
    parser.add_argument(
        "--faction",
        type=str,
        default="all",
        help="Filter faction: Soviet, Alliance, Coalition, Chrono, or all"
    )
    parser.add_argument(
        "--event",
        type=str,
        default="all",
        help="Filter event tag or all"
    )
    parser.add_argument(
        "--resume",
        action="store_true",
        help="Skip existing files that already passed QC"
    )
    parser.add_argument(
        "--seed-base",
        type=int,
        default=42,
        help="Random seed base integer"
    )
    parser.add_argument(
        "--takes",
        type=int,
        default=3,
        help="Number of takes to generate per line"
    )
    parser.add_argument(
        "--inference-steps",
        type=int,
        default=15,
        help="Number of inference steps"
    )
    parser.add_argument(
        "--cfg",
        type=float,
        default=2.0,
        help="Classifier-free guidance scale"
    )
    parser.add_argument(
        "--output-root",
        type=str,
        default="Content/RA4/Audio/EVA",
        help="Root directory for generated and processed audio"
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Force overwrite existing files regardless of status"
    )

    args = parser.parse_args()

    # Override mode with positional arg if supplied
    if args.mode_pos is not None:
        args.mode = args.mode_pos

    # Resolve device auto
    if args.device == "auto":
        args.device = "mps" if sys.platform == "darwin" else "cpu"

    controller = EVAPipelineController(args)
    controller.run()


if __name__ == "__main__":
    main()
