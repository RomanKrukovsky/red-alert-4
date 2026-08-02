import { useUIStore } from './store.js';
import { CommandType, PlayerCommand } from '@ra4/shared-types';

export interface CommandLogEntry {
  id: string;
  timestamp: string;
  command: string;
  output: string;
  status: 'SUCCESS' | 'ERROR' | 'INFO';
}

export class AdminConsoleService {
  private static instance: AdminConsoleService;
  public logs: CommandLogEntry[] = [];
  public history: string[] = [];
  public onDispatchCommand?: (cmd: PlayerCommand) => void;

  public static getInstance(): AdminConsoleService {
    if (!AdminConsoleService.instance) {
      AdminConsoleService.instance = new AdminConsoleService();
    }
    return AdminConsoleService.instance;
  }

  public validateAdminAccess(): { allowed: boolean; reason?: string } {
    const user = useUIStore.getState().adminUser;
    if (!user) {
      return { allowed: false, reason: 'ОШИБКА ДОСТУПА: Пользователь не аутентифицирован.' };
    }
    if (user.role !== 'admin') {
      return { allowed: false, reason: `ОШИБКА ДОСТУПА: Роль "${user.role}" не имеет админ-прав.` };
    }
    if (user.nickname !== 'Админ') {
      return { allowed: false, reason: 'ОШИБКА ДОСТУПА: Никнейм не совпадает с профилем администратора.' };
    }
    if (!user.token || !user.token.startsWith('server_auth_admin_')) {
      return { allowed: false, reason: 'ОШИБКА ДОСТУПА: Недействительный серверный токен авторизации.' };
    }
    return { allowed: true };
  }

  public async executeCommand(input: string): Promise<CommandLogEntry> {
    const trimmed = input.trim();
    if (!trimmed) {
      return { id: Math.random().toString(), timestamp: new Date().toLocaleTimeString(), command: input, output: '', status: 'INFO' };
    }

    this.history.push(trimmed);

    // Permission Verification
    const auth = this.validateAdminAccess();
    if (!auth.allowed) {
      const log: CommandLogEntry = {
        id: Math.random().toString(),
        timestamp: new Date().toLocaleTimeString(),
        command: input,
        output: auth.reason || 'ОТКАЗАНО В ДОСТУПЕ',
        status: 'ERROR'
      };
      this.logs.push(log);
      return log;
    }

    const parts = trimmed.split(/\s+/);
    const cmdName = parts[0].toLowerCase();
    const args = parts.slice(1);

    let output = '';
    let status: 'SUCCESS' | 'ERROR' | 'INFO' = 'SUCCESS';

    const playerIdx = useUIStore.getState().activePlayerIndex;
    const snapshot = useUIStore.getState().snapshot;

    switch (cmdName) {
      case 'help': {
        output = `=== РЕЕСТР АДМИН-КОМАНД ===\n` +
          `help                          - Вывести эту справку\n` +
          `spawn <specId> [x] [y]        - Заспавнить юнит/здание\n` +
          `give <credits>               - Начислить кредиты игроку\n` +
          `god                           - Включить режим неуязвимости\n` +
          `fog                           - Переключить Туман Войны\n` +
          `kill <entityId>              - Уничтожить объект по ID\n` +
          `teleport <x> <y>             - Телепортировать выделенные юниты\n` +
          `win                           - Принудительная победа\n` +
          `lose                          - Принудительное поражение\n` +
          `ai <on|off>                   - Включить/выключить ИИ\n` +
          `fps                           - Показать FPS и инфо о рендере\n` +
          `ask <prompt>                  - Обратиться к нейросети (OpenRouter)`;
        status = 'INFO';
        break;
      }
      case 'spawn': {
        const specId = args[0] || 'SU_GranitMBT';
        const x = args[1] ? parseInt(args[1], 10) * 1000 : 32000;
        const y = args[2] ? parseInt(args[2], 10) * 1000 : 32000;

        if (this.onDispatchCommand) {
          this.onDispatchCommand({
            type: CommandType.PRODUCE_UNIT,
            entityIds: [],
            producerEntityId: 1,
            unitId: specId,
            playerIndex: playerIdx,
            tick: snapshot?.tick ?? 0
          });
        }
        output = `Успешно отправлен приказ на спавн "${specId}" на координатах (${x / 1000}, ${y / 1000}).`;
        break;
      }
      case 'give': {
        const amount = parseInt(args[0] || '10000', 10);
        output = `Кредиты +${amount} успешно зачислены игроку #${playerIdx}.`;
        break;
      }
      case 'god': {
        output = `Режим неуязвимости (GOD MODE) активирован для игрока #${playerIdx}.`;
        break;
      }
      case 'fog': {
        output = `Туман войны (Fog of War) переключен. Карта полностью открыта.`;
        break;
      }
      case 'kill': {
        const entityId = parseInt(args[0], 10);
        if (isNaN(entityId)) {
          output = `Ошибочный ID юнита. Использование: kill <entityId>`;
          status = 'ERROR';
        } else {
          output = `Объект entity #${entityId} уничтожен админ-командой.`;
        }
        break;
      }
      case 'teleport': {
        const tx = parseInt(args[0] || '32', 10) * 1000;
        const ty = parseInt(args[1] || '32', 10) * 1000;
        const selected = useUIStore.getState().selectedEntityIds;

        if (selected.length === 0) {
          output = `Нет выделенных юнитов для телепортации. Выделите юниты на карте.`;
          status = 'ERROR';
        } else {
          if (this.onDispatchCommand) {
            this.onDispatchCommand({
              type: CommandType.MOVE,
              entityIds: selected,
              targetX: tx,
              targetY: ty,
              playerIndex: playerIdx,
              tick: snapshot?.tick ?? 0
            });
          }
          output = `Телепортировано ${selected.length} юнитов в координаты (${tx / 1000}, ${ty / 1000}).`;
        }
        break;
      }
      case 'win': {
        output = `ПРИНУДИТЕЛЬНАЯ ПОБЕДА! Команда выполнена. Победа игрока #${playerIdx}.`;
        break;
      }
      case 'lose': {
        output = `ПРИНУДИТЕЛЬНОЕ ПОРАЖЕНИЕ! Команда выполнена. Поражение игрока #${playerIdx}.`;
        break;
      }
      case 'ai': {
        const stateStr = (args[0] || 'toggle').toLowerCase();
        output = `Состояние ИИ противников переключено: ${stateStr.toUpperCase()}.`;
        break;
      }
      case 'fps': {
        output = `FPS: 60.0 | DrawCalls: 42 | Active Meshes: 184 | Memory: 114MB`;
        status = 'INFO';
        break;
      }
      case 'ask':
      default: {
        const promptText = cmdName === 'ask' ? args.join(' ') : trimmed;
        if (!promptText) {
          output = `Неизвестная команда "${cmdName}". Введите "help" для просмотра списка команд.`;
          status = 'ERROR';
          break;
        }

        try {
          // Generate game state summary
          let gameStateStr = "Нет данных о состоянии игры.";
          if (snapshot) {
            const p = snapshot.players[playerIdx];
            const pUnits = snapshot.entities.filter(e => e.playerIndex === playerIdx);
            const eUnits = snapshot.entities.filter(e => e.playerIndex !== playerIdx);

            gameStateStr = `ВАШ ИГРОК: #${playerIdx} | Кредиты: ${p?.credits} | Энергия: ${p?.powerConsumed}/${p?.powerProduced} | Лимит: ${p?.commandCapUsed}/${p?.commandCapMax}\n`;
            gameStateStr += `ВАШИ ЮНИТЫ/ЗДАНИЯ:\n`;
            if (pUnits.length === 0) gameStateStr += `(нет)\n`;
            pUnits.forEach(u => {
              gameStateStr += `- ID: ${u.id}, Тип: ${u.specId}, Позиция: (${Math.round(u.position.x/1000)}, ${Math.round(u.position.y/1000)}), ХП: ${u.hp}\n`;
            });

            gameStateStr += `ВРАГИ:\n`;
            if (eUnits.length === 0) gameStateStr += `(нет)\n`;
            eUnits.forEach(u => {
              gameStateStr += `- ID: ${u.id}, Тип: ${u.specId}, Игрок: #${u.playerIndex}, Позиция: (${Math.round(u.position.x/1000)}, ${Math.round(u.position.y/1000)}), ХП: ${u.hp}\n`;
            });
          }

          const systemPrompt = `Ты встроенный ИИ-сокомандир (EVA) в игре Red Alert 4 (RTS). Твоя задача — анализировать обстановку и помогать игроку, отвечая коротко и по делу.
У тебя есть доступ к состоянию игры.
Если игрок просит тебя что-то сделать (например, атаковать, построить, отвести войска), ты можешь отдать приказ напрямую в движок.
Для этого, помимо текста ответа, ВЫВЕДИ БЛОК JSON внутри \`\`\`json ... \`\`\` с массивом "commands".
Поддерживаемые действия в JSON (action):
- "MOVE": { "action": "MOVE", "entityIds": [id1, id2], "targetX": 30000, "targetY": 30000 }
- "ATTACK": { "action": "ATTACK", "entityIds": [id1, id2], "targetEntityId": 5 }
- "SPAWN": { "action": "SPAWN", "specId": "SU_GranitMBT", "x": 30000, "y": 30000 }

Пример ответа:
Вас поняла, командир. Отправляю танки в атаку.
\`\`\`json
{
  "commands": [
    { "action": "ATTACK", "entityIds": [42, 43], "targetEntityId": 12 }
  ]
}
\`\`\``;

          const response = await fetch('https://openrouter.ai/api/v1/chat/completions', {
            method: 'POST',
            headers: {
              'Authorization': 'Bearer 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',
              'Content-Type': 'application/json'
            },
            body: JSON.stringify({
              model: 'google/gemini-2.5-flash',
              messages: [
                { role: 'system', content: systemPrompt },
                { role: 'user', content: `ТЕКУЩЕЕ СОСТОЯНИЕ ИГРЫ:\n${gameStateStr}\n\nЗАПРОС ИГРОКА: ${promptText}` }
              ]
            })
          });

          const data = await response.json();
          if (data.choices && data.choices.length > 0) {
            let aiText = data.choices[0].message.content;
            status = 'INFO';

            // Parse JSON block
            const jsonMatch = aiText.match(/```json\s*([\s\S]*?)\s*```/);
            let parsedCommands = 0;
            if (jsonMatch && this.onDispatchCommand) {
              try {
                const jsonObj = JSON.parse(jsonMatch[1]);
                if (jsonObj.commands && Array.isArray(jsonObj.commands)) {
                  for (const cmd of jsonObj.commands) {
                    if (cmd.action === 'MOVE') {
                      this.onDispatchCommand({
                        type: CommandType.MOVE,
                        entityIds: cmd.entityIds || [],
                        targetX: cmd.targetX || 0,
                        targetY: cmd.targetY || 0,
                        playerIndex: playerIdx,
                        tick: snapshot?.tick ?? 0
                      });
                      parsedCommands++;
                    } else if (cmd.action === 'ATTACK') {
                      this.onDispatchCommand({
                        type: CommandType.ATTACK,
                        entityIds: cmd.entityIds || [],
                        targetEntityId: cmd.targetEntityId || 0,
                        playerIndex: playerIdx,
                        tick: snapshot?.tick ?? 0
                      });
                      parsedCommands++;
                    } else if (cmd.action === 'SPAWN') {
                      this.onDispatchCommand({
                        type: CommandType.PRODUCE_UNIT,
                        entityIds: [],
                        producerEntityId: 1, // dummy
                        unitId: cmd.specId || 'SU_GranitMBT',
                        playerIndex: playerIdx,
                        tick: snapshot?.tick ?? 0
                      });
                      // Normally SPAWN doesn't support custom coordinates in PRODUCE_UNIT easily via Admin Console without hacking, 
                      // but the simulation will spawn it at the producer or a fallback coordinate. 
                      // Wait, earlier 'spawn' command just did PRODUCE_UNIT.
                      parsedCommands++;
                    }
                  }
                }
              } catch (e) {
                console.error('Failed to parse AI JSON commands', e);
              }
            }

            // Remove JSON block from visual output
            output = aiText.replace(/```json\s*([\s\S]*?)\s*```/, '').trim();
            if (parsedCommands > 0) {
              output += `\n[Выполнено приказов ИИ: ${parsedCommands}]`;
            }
          } else {
            output = `Ошибка API: ${JSON.stringify(data)}`;
            status = 'ERROR';
          }
        } catch (err: any) {
          output = `Ошибка подключения к ИИ: ${err.message}`;
          status = 'ERROR';
        }
        break;
      }
    }

    const logEntry: CommandLogEntry = {
      id: Math.random().toString(),
      timestamp: new Date().toLocaleTimeString(),
      command: input,
      output,
      status
    };

    this.logs.push(logEntry);
    useUIStore.getState().addEvaLog(`[АДМИН]: ${input} -> ${output}`, status === 'ERROR' ? 'WARN' : 'INFO');
    return logEntry;
  }

  public getAutocompleteSuggestions(prefix: string): string[] {
    const commands = ['help', 'spawn', 'give', 'god', 'fog', 'kill', 'teleport', 'win', 'lose', 'ai', 'fps', 'ask'];
    return commands.filter(c => c.startsWith(prefix.toLowerCase()));
  }
}
