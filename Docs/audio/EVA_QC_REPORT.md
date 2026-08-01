# Report po avtomaticheskoy proverke kachestva (EVA Quality Control Report) — Red Alert 4

## 1. Svodka audita kachestva

Avtomaticheskiy kontrol kachestva (Automated Quality Control Pass) proveden for vsey rabochey vyborki finalnykh golosovykh replik EVA vsekh 4 fraktsiy (`SU`, `AL`, `CO`, `CH`).

* **Total proanalizirovano faylov:** 40
* **Success proshlo QC (APPROVED_AUTO):** 40 (100.0%)
* **Faylov s preduprezhdeniyami (NEEDS_LISTENING):** 0 (0.0%)
* **Oshibok / Otkloneno (REJECTED / MISSING):** 0 (0.0%)

---

## 2. Proverennye audiometricheskie kriterii

1. **Format i chastota diskretizatsii:** 48 000 Gts, Mono, 24-bit PCM (WAV).
2. **Integrirovannaya gromkost (Integrated Loudness):** Tselevoe Value **-18.0 LUFS** ($\pm 0.8$ LUFS).
   * Minimalnaya zafiksirovannaya gromkost: -18.66 LUFS
   * Maksimalnaya zafiksirovannaya gromkost: -18.00 LUFS
3. **Pikovyy uroven (True Peak Limit):** Maksimum **-1.0 dBTP** (fakticheski zafiksirovano v diapazone ot -1.92 dBTP before -1.65 dBTP).
4. **Obrezka tishiny (Silence Padding):**
   * Nachalnaya tishina: 80–140 ms (tsel: 100 ms).
   * Konechnaya tishina: 160–260 ms (tsel: 200 ms).
5. **Klipping i kliki:** 0 obnaruzheno.

---

## 3. Tablitsa rezultatov po fraktsiyam

| Fraktsiya | Golos | verified WAV | Approved Auto | Avg LUFS | Max Peak (dBTP) | QC Status |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: |
| **Soviet Union** | `EVA_SU_KONTUR` | 10 | 10 | -18.21 LUFS | -1.65 dBTP | **PASSED** |
| **Alliance** | `EVA_AL_ASTRA` | 10 | 10 | -18.53 LUFS | -1.92 dBTP | **PASSED** |
| **Vostochnaya Coalition** | `EVA_CO_HARMONIA` | 10 | 10 | -18.33 LUFS | -1.92 dBTP | **PASSED** |
| **Khronolegion** | `EVA_CH_MOIRA` | 10 | 10 | -18.41 LUFS | -1.92 dBTP | **PASSED** |

---

## 4. Log proverok

Tablitsa detalnykh proverok sokhranena v `Content/RA4/Audio/Generated/eva_qc_report.csv` i importirovana v Manifest Unreal Engine.