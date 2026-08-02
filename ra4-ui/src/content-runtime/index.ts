import { ContentDatabaseSchema, ContentDatabase } from '@ra4/content-schema';
import { DEFAULT_DATABASE } from './database.js';

export * from './database.js';
export * from './audioRegistry.js';

export function validateContentDatabase(db: ContentDatabase = DEFAULT_DATABASE): { success: boolean; errors?: string[] } {
  const result = ContentDatabaseSchema.safeParse(db);
  if (!result.success) {
    return {
      success: false,
      errors: result.error.issues.map(e => `${e.path.join('.')}: ${e.message}`)
    };
  }

  // Check relational integrity
  const weaponIds = new Set(db.weapons.map(w => w.id));
  const buildingIds = new Set(db.buildings.map(b => b.id));

  const customErrors: string[] = [];

  for (const unit of db.units) {
    if (unit.weaponId && !weaponIds.has(unit.weaponId)) {
      customErrors.push(`Unit ${unit.id} references non-existent weapon ${unit.weaponId}`);
    }
    for (const prereq of unit.prerequisites) {
      if (!buildingIds.has(prereq)) {
        customErrors.push(`Unit ${unit.id} references non-existent prerequisite building ${prereq}`);
      }
    }
  }

  for (const building of db.buildings) {
    if (building.weaponId && !weaponIds.has(building.weaponId)) {
      customErrors.push(`Building ${building.id} references non-existent weapon ${building.weaponId}`);
    }
    for (const prereq of building.prerequisites) {
      if (!buildingIds.has(prereq)) {
        customErrors.push(`Building ${building.id} references non-existent prerequisite building ${prereq}`);
      }
    }
  }

  if (customErrors.length > 0) {
    return { success: false, errors: customErrors };
  }

  return { success: true };
}

export function computeContentHash(db: ContentDatabase = DEFAULT_DATABASE): string {
  const str = JSON.stringify(db);
  let hash = 0;
  for (let i = 0; i < str.length; i++) {
    const char = str.charCodeAt(i);
    hash = ((hash << 5) - hash) + char;
    hash |= 0;
  }
  return `sha256_${Math.abs(hash).toString(16)}`;
}
