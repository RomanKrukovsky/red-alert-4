# CI/CD & Build Infrastructure Report (`CICD_AND_INFRASTRUCTURE.md`)

**Document Version**: 8.0  
**Evaluation Date**: August 4, 2026  
**Status**: **PIPELINE ACTIVE & VERIFIED**  

---

## 1. GitHub Actions Pipeline Architecture (`.github/workflows/core.yml`)

The continuous integration pipeline automates formatting checks, static analysis, headless compilation, unit testing, and packaged build creation.

```
[ Push / Pull Request ]
          │
          ├────────────────────────┬────────────────────────┐
          ▼                        ▼                        ▼
[ Code Formatting & Lint ]   [ Headless C++ Build ]   [ Content Validation ]
          │                        │                        │
          │                        ▼                        │
          │              [ Run 395 C++ Unit Tests ]         │
          │                        │                        │
          └────────────────────────┴────────────────────────┘
                                   │
                                   ▼
                         [ Build Verification Gate ]
                                   │
                                   ▼
                         [ Shipping Package Build ]
                                   │
                                   ▼
                         [ Artifact & Symbol Archive ]
```

---

## 2. Pipeline Stages & Enforcement Policies

1. **Formatting & Linting**: Clang-Format check ensures code conforms to project formatting rules.
2. **Headless Compilation**: `Tools/HeadlessBuild/CMakeLists.txt` compiles standalone C++ static libraries and test executables under Clang/GCC.
3. **Automated Unit & Lockstep Tests**: Executes `./RA4Tests && ./RA4AITests && ./RA4InputTests && ./RA4PresentationTests`. 100% pass required.
4. **Content Validation**: Schema validator checks `ra4_content.normalized.json` and campaign data files for zero broken links.
5. **Symbol Retention**: Debug symbols (`.dSYM` / `.pdb`) are archived per release build for crash dump stack unwinding.
