export const ZONE_COLORS = {
  Colegio: '#f59e0b',
  Universidad: '#8b5cf6',
  Hospital: '#dc2626',
  Entretenimiento: '#0891b2',
};

export const zoneColor = (tipo) => ZONE_COLORS[tipo] || '#6b7280';
