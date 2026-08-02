import { CommandType } from './enums.js';

export interface BaseCommand {
  type: CommandType;
  entityIds: number[];
  playerIndex: number;
  tick: number;
}

export interface MoveCommand extends BaseCommand {
  type: CommandType.MOVE;
  targetX: number; // scaled int
  targetY: number; // scaled int
}

export interface AttackCommand extends BaseCommand {
  type: CommandType.ATTACK;
  targetEntityId: number;
}

export interface AttackMoveCommand extends BaseCommand {
  type: CommandType.ATTACK_MOVE;
  targetX: number;
  targetY: number;
}

export interface StopCommand extends BaseCommand {
  type: CommandType.STOP;
}

export interface HoldCommand extends BaseCommand {
  type: CommandType.HOLD;
}

export interface BuildStructureCommand extends BaseCommand {
  type: CommandType.BUILD_STRUCTURE;
  structureId: string;
  gridX: number;
  gridY: number;
}

export interface ProduceUnitCommand extends BaseCommand {
  type: CommandType.PRODUCE_UNIT;
  producerEntityId: number;
  unitId: string;
}

export interface CancelProductionCommand extends BaseCommand {
  type: CommandType.CANCEL_PRODUCTION;
  producerEntityId: number;
  queueIndex: number;
}

export interface SellStructureCommand extends BaseCommand {
  type: CommandType.SELL_STRUCTURE;
  structureEntityId: number;
}

export interface RepairStructureCommand extends BaseCommand {
  type: CommandType.REPAIR_STRUCTURE;
  structureEntityId: number;
}

export interface UseAbilityCommand extends BaseCommand {
  type: CommandType.USE_ABILITY;
  abilityId: string;
  targetX?: number;
  targetY?: number;
  targetEntityId?: number;
}

export interface CaptureBuildingCommand extends BaseCommand {
  type: CommandType.CAPTURE_BUILDING;
  targetStructureId: number;
}

export interface GatherCommand extends BaseCommand {
  type: CommandType.GATHER;
  resourceNodeId?: string;
}

export interface DepositOreCommand extends BaseCommand {
  type: CommandType.DEPOSIT_ORE;
  refineryEntityId: number;
}

export interface SurrenderCommand extends BaseCommand {
  type: CommandType.SURRENDER;
}

export type PlayerCommand =
  | MoveCommand
  | AttackCommand
  | AttackMoveCommand
  | StopCommand
  | HoldCommand
  | BuildStructureCommand
  | ProduceUnitCommand
  | CancelProductionCommand
  | SellStructureCommand
  | RepairStructureCommand
  | UseAbilityCommand
  | CaptureBuildingCommand
  | DepositOreCommand
  | GatherCommand
  | SurrenderCommand;
