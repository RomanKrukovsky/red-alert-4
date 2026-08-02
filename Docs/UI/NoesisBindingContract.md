# Red Alert 4: NoesisGUI Data Binding Contract & ViewModel Specification

## 1. Data Binding Rules

1. **Zero Direct Actor References**: XAML bindings must bind only to `UObject` ViewModels (`URA4*ViewModel`).
2. **Notification-Driven Updates**: ViewModels implement `INotifyPropertyChanged` and fire property changes ONLY when values mutate. No per-frame tick polling in ViewModels.
3. **Immutable Snapshots**: Game simulation state is converted into immutable struct/object snapshots before passing to the UI thread.
4. **Command Execution**: All user actions map to `ICommand` or `FUIAction` implementations that route commands through `PlayerController` or `URA4CommandBus`.

---

## 2. ViewModel Property Contracts

### 2.1 `URA4HUDViewModel`
* `Credits` (`int32`, ReadOnly, Notify)
* `Power` (`int32`, ReadOnly, Notify)
* `Intel` (`int32`, ReadOnly, Notify)
* `SelectionType` (`EUIRefSelectionType`, ReadOnly, Notify)
* `SelectedUnitCount` (`int32`, ReadOnly, Notify)
* `QueueItems` (`TArray<FUIProductionItemSnapshot>`, ReadOnly, Notify)
* `IssueCommand` (`ICommand*`, Execute)

### 2.2 `URA4SkirmishLobbyViewModel`
* `PlayerList` (`TArray<FUILobbyPlayerSnapshot>`, ReadOnly, Notify)
* `SelectedMapName` (`FText`, ReadOnly, Notify)
* `ChatHistory` (`TArray<FUIChatMessage>`, ReadOnly, Notify)
* `StartMatchCommand` (`ICommand*`, Execute)
* `SendChatMessageCommand` (`ICommand*`, Execute)
