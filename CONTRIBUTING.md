# 🤝 Contributing to Red Alert 4

Thank you for your interest in contributing to Red Alert 4! This document provides guidelines for contributing to the project.

---

## 🎯 How to Contribute

### **For Developers**

#### 1. Set Up Your Development Environment

```bash
# Clone the repository
git clone https://github.com/your-org/red-alert-4.git
cd red-alert-4

# Install Unreal Engine 5.3
# Add Unreal Engine to your PATH
# Example: /Users/Shared/Epic Games/UE_5.3/Engine/Binaries/ThirdParty/NotForLicensees

# Install Git LFS
git lfs install

# Install development dependencies
pip install -r requirements.txt  # If you have any Python dependencies
```

#### 2. Build the Project

```bash
# Build for development
./BuildScripts/build_all.sh Win64

# Build for shipping
./BuildScripts/build_all.sh Win64 --shipping

# Check build artifacts
ls -la build/Win64/RedAlert4/Content/Paks/
```

#### 3. Run Tests

```bash
# Run unit tests
python Tests/run_tests.py

# Run Unreal Automation Tests
UnrealEditor RedAlert4.uproject -ExecCmds="Automation RunTests RedAlert4" -nullrhi -unattended

# Check test coverage
python Tools/code_metrics.py
```

---

### **Code Style Guidelines**

#### **C++ Naming Conventions**

```cpp
// Classes
class FRedAlert4PlayerManager { ... }

// Structs
struct FUnitStats { ... }

// Enums
enum class EResourceType { Ore, Crystal, Power };

// Variables
int32 CurrentCredits;
bool bIsActive;

// Constants
const int32 MAX_PLAYERS = 8;

// Functions
void CalculateProductionCapacity();
bool CanAfford(int32 Cost);

// Private members
int32 _currentCredits;
FString _unitName;
```

#### **File Structure**

```
Source/
├── Core/                  # Core systems (Economy, Simulation, etc.)
│   ├── Public/            # Public headers
│   ├── Private/           # Private implementation
│   └── Tests/             # Unit tests
├── Gameplay/              # Gameplay systems (Units, Buildings, etc.)
├── AI/                    # Artificial Intelligence
├── Simulation/            # Game simulation
├── Audio/                 # Audio systems
└── Content/               # Content files
```

#### **Documentation Requirements**

Every public class and method must have:

```cpp
/**
 * Calculates the total production capacity for a given resource type.
 * 
 * @param ResourceType Type of resource to calculate
 * @param bIncludeUpgrades Whether to include building upgrades in calculation
 * @return Total production capacity
 * @see UResourceManager, AProductionBuilding
 */
int32 CalculateProductionCapacity(EResourceType ResourceType, bool bIncludeUpgrades = true);
```

---

### **Development Workflow**

#### 1. Create a Feature Branch

```bash
# Create a new branch for your feature
git checkout -b feature/unit-movement-system

# Or for a bug fix
git checkout -b bugfix/resource-leak
```

#### 2. Make Your Changes

```cpp
// Example: Adding a new unit type
UCLASS()
class REDALERT4_API AInfantryUnit : public ACombatUnit
{
    GENERATED_BODY()

public:
    /**
     * Fire weapon at target position.
     * @param TargetPosition Position to attack
     */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void FireAt(const FVector& TargetPosition);

private:
    UPROPERTY(EditAnywhere, Category = "Combat")
    float WeaponRange;

    UPROPERTY(EditAnywhere, Category = "Combat")
    int32 DamagePerShot;
};
```

#### 3. Test Your Changes

```bash
# Build the project
./BuildScripts/build_all.sh Win64

# Run tests
python Tests/run_tests.py

# Test in editor
UnrealEditor RedAlert4.uproject

# Test specific functionality
```

#### 4. Commit Your Changes

```bash
# Stage your changes
git add Source/Core/Public/Economy/ResourceSystem.h

# Write a good commit message
git commit -m "feat(economy): Add resource cap system

- Add ResourceCap component
- Implement cap checking logic
- Add unit tests for resource limits

Fixes #123"
```

**Commit Message Format:**
```
<type>(<scope>): <description>

[type]: feat, fix, docs, style, refactor, test, chore
[scope]: economy, simulation, ai, audio, ui, build, ci, etc.

Example types:
- feat: A new feature
- fix: A bug fix
- docs: Documentation only changes
- style: Formatting, missing semicolons, etc
- refactor: A code change that neither fixes a bug nor adds a feature
- test: Adding missing tests
- chore: Changes to the build process or auxiliary tools
```

#### 5. Push and Create Pull Request

```bash
# Push your changes
git push origin feature/unit-movement-system

# Create a Pull Request on GitHub
# Include:
# - Description of changes
# - Screenshots if UI changes
# - Testing performed
# - Related issues
```

---

### **Pull Request Guidelines**

#### **Required Checks**

✅ **Before submitting a PR, ensure:**

- [ ] Code builds successfully on all platforms
- [ ] All existing tests pass
- [ ] New tests added for new functionality
- [ ] Code follows project style guidelines
- [ ] Documentation updated (if applicable)
- [ ] No console warnings or errors
- [ ] Performance impact assessed
- [ ] Memory leaks checked (use Unreal Insights)

#### **PR Template**

```markdown
## Description

[Provide a clear description of the changes]

## Related Issues

- Fixes #123
- Related to #456

## Changes Made

- [ ] Added new feature
- [ ] Fixed bug
- [ ] Updated documentation
- [ ] Refactored code
- [ ] Added tests

## Testing

- [ ] Unit tests added/updated
- [ ] Integration tests passed
- [ ] Manual testing performed
- [ ] Performance tested

## Screenshots/Videos

[If applicable, add screenshots or videos]

## Checklist

- [ ] My code follows the project's style guidelines
- [ ] I have performed a self-review of my code
- [ ] I have commented my code, particularly in hard-to-understand areas
- [ ] I have made corresponding changes to the documentation
- [ ] My changes generate no new warnings
- [ ] I have added tests that prove my fix is effective or that my feature works
- [ ] New and existing unit tests pass locally with my changes
- [ ] Any dependent changes have been merged and published in downstream modules
```

---

### **Code Review Process**

#### **What to Expect**

1. **Automated Checks:** GitHub Actions will run CI pipeline
2. **Code Review:** Maintainers will review your code
3. **Feedback:** You'll receive feedback and suggestions
4. **Approval:** Once approved, your PR will be merged

#### **Review Criteria**

✅ **Code Quality:**
- Follows project style guidelines
- Well-documented
- Proper error handling
- No code smells or anti-patterns

✅ **Functionality:**
- Works as intended
- Handles edge cases
- No regressions
- Performance acceptable

✅ **Testing:**
- Tests cover critical paths
- Tests are maintainable
- Tests run in CI

✅ **Documentation:**
- Code is documented
- Public APIs documented
- README updated if needed

---

### **Reporting Issues**

#### **Bug Reports**

```markdown
## Description

[Clear and concise description of the bug]

## Steps to Reproduce

1. Go to '...'
2. Click on '....'
3. Scroll down to '....'
4. See error

## Expected Behavior

[What you expected to happen]

## Actual Behavior

[What actually happened]

## Screenshots/Videos

[If applicable, add screenshots or videos]

## Environment

- **Unreal Engine Version:** 5.3
- **Platform:** Windows 11
- **Build Configuration:** Development
- **Branch:** main

## Additional Context

[Any other context about the problem]
```

#### **Feature Requests**

```markdown
## Description

[Clear description of the feature]

## Problem Statement

[What problem does this solve?]

## Proposed Solution

[How should this be implemented?]

## Alternatives Considered

[Any alternative solutions?]

## Additional Context

[Any other context or screenshots]
```

---

### **Getting Help**

#### **Questions**

- Check the [README.md](README.md)
- Review the [CLAUDE.md](CLAUDE.md)
- Look at existing issues and PRs
- Check Unreal Engine documentation

#### **Discussions**

- Join our Discord: [https://discord.gg/red-alert-4](https://discord.gg/red-alert-4)
- Ask questions in GitHub Discussions
- Check the wiki for detailed guides

#### **Contact**

- **Project Lead:** [Your Name]
- **Email:** your-email@example.com
- **Discord:** @your-discord-name

---

### **Development Tips**

#### **Unreal Engine Tips**

```cpp
// Use UE_LOG for debugging
UE_LOG(LogTemp, Log, TEXT("Player position: %s"), *PlayerPosition.ToString());

// Use UE_LOG with different verbosity levels
UE_LOG(LogTemp, Warning, TEXT("Resource depleted!"));
UE_LOG(LogTemp, Error, TEXT("Failed to load asset!"));

// Use GEngine->AddOnScreenDebugMessage for in-game debugging
if (GEngine)
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Unit spawned!"));
}

// Use CHECK and ensure for runtime checks
check(Unit != nullptr);
ensure(Health > 0);
```

#### **Performance Tips**

```cpp
// Use TArray for dynamic collections
TArray<AUnit*> Units;

// Use TMap for key-value pairs
TMap<FString, int32> ResourceCounts;

// Use TSharedPtr for shared ownership
TSharedPtr<FObjectPool> UnitPool;

// Use const references for function parameters
void ProcessUnits(const TArray<AActor*>& Units);

// Use UPROPERTY for Unreal reflection
UPROPERTY(EditAnywhere, Category = "Combat")
int32 MaxHealth;
```

---

### **Code of Conduct**

We expect all contributors to follow our [Code of Conduct](CODE_OF_CONDUCT.md). Be respectful, inclusive, and professional.

---

### **License**

By contributing, you agree that your contributions will be licensed under the project's [LICENSE](LICENSES.md).

---

### **Acknowledgements**

Thank you to all our contributors! 🎉

---

**📝 Last Updated:** August 18, 2026  
**🔄 Version:** 1.0  
**📚 Related Documents:** [README.md](README.md), [IMPROVEMENT_PLAN.md](IMPROVEMENT_PLAN.md)
