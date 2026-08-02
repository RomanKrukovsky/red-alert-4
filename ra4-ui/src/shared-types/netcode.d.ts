import { FactionId, MatchState, PlayerType } from './enums.js';
import { PlayerCommand } from './commands.js';
import { WorldSnapshot } from './simulation.js';
export interface PlayerSlot {
    index: number;
    name: string;
    type: PlayerType;
    factionId: FactionId;
    team: number;
    color: string;
    isReady: boolean;
    isConnected: boolean;
}
export interface LobbyState {
    roomId: string;
    mapId: string;
    matchState: MatchState;
    hostIndex: number;
    slots: PlayerSlot[];
    contentVersionHash: string;
}
export type ServerMessageType = 'LOBBY_STATE' | 'MATCH_START' | 'TICK_FRAME' | 'STATE_SNAPSHOT' | 'CHECKSUM_MISMATCH' | 'GAME_OVER' | 'ERROR';
export interface ServerMessageLobbyState {
    type: 'LOBBY_STATE';
    state: LobbyState;
}
export interface ServerMessageMatchStart {
    type: 'MATCH_START';
    seed: number;
    tickRate: number;
    initialSnapshot: WorldSnapshot;
}
export interface ServerMessageTickFrame {
    type: 'TICK_FRAME';
    tick: number;
    commands: PlayerCommand[];
}
export interface ServerMessageSnapshot {
    type: 'STATE_SNAPSHOT';
    snapshot: WorldSnapshot;
}
export interface ServerMessageChecksumMismatch {
    type: 'CHECKSUM_MISMATCH';
    tick: number;
    serverChecksum: number;
    clientChecksum: number;
}
export interface ServerMessageGameOver {
    type: 'GAME_OVER';
    winnerTeam: number;
    winningPlayerIndices: number[];
    reason: string;
}
export interface ServerMessageError {
    type: 'ERROR';
    message: string;
}
export type ServerMessage = ServerMessageLobbyState | ServerMessageMatchStart | ServerMessageTickFrame | ServerMessageSnapshot | ServerMessageChecksumMismatch | ServerMessageGameOver | ServerMessageError;
export type ClientMessageType = 'JOIN_LOBBY' | 'SET_SLOT' | 'SET_MAP' | 'SET_READY' | 'START_MATCH' | 'SUBMIT_COMMAND' | 'CHECKSUM_REPORT' | 'RECONNECT';
export interface ClientMessageJoinLobby {
    type: 'JOIN_LOBBY';
    playerName: string;
    roomId?: string;
}
export interface ClientMessageSetSlot {
    type: 'SET_SLOT';
    slotIndex: number;
    factionId?: FactionId;
    type?: PlayerType;
    team?: number;
}
export interface ClientMessageSetMap {
    type: 'SET_MAP';
    mapId: string;
}
export interface ClientMessageSetReady {
    type: 'SET_READY';
    isReady: boolean;
}
export interface ClientMessageStartMatch {
    type: 'START_MATCH';
}
export interface ClientMessageSubmitCommand {
    type: 'SUBMIT_COMMAND';
    command: PlayerCommand;
}
export interface ClientMessageChecksumReport {
    type: 'CHECKSUM_REPORT';
    tick: number;
    checksum: number;
}
export interface ClientMessageReconnect {
    type: 'RECONNECT';
    roomId: string;
    playerIndex: number;
    lastTick: number;
}
export type ClientMessage = ClientMessageJoinLobby | ClientMessageSetSlot | ClientMessageSetMap | ClientMessageSetReady | ClientMessageStartMatch | ClientMessageSubmitCommand | ClientMessageChecksumReport | ClientMessageReconnect;
//# sourceMappingURL=netcode.d.ts.map