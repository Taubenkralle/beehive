# Beehive — Wiki

**Beehive** is a biologically realistic 2D honey bee colony simulator written in C with raylib.

The simulation models a living bee colony with real-world mechanics: bee development, task allocation, resource economics, thermoregulation, pheromone communication, and weather.

---

## Wiki Pages

| Page | Contents |
|------|----------|
| [Bee Lifecycle](Bee-Lifecycle) | Egg → Larva → Pupa → Adult, development timers |
| [Roles & Tasks](Roles-and-Tasks) | All 12 worker roles, the job queue system |
| [Resources](Resources) | Honey, nectar, pollen, wax, water — how they flow |
| [Brood Care & Thermoregulation](Brood-Care) | Keeping 35 °C in the brood nest |
| [Pheromone System](Pheromone-System) | Alarm, trail, Nasonov — how bees communicate |
| [Outside World & Weather](Outside-World) | Forage sources, day/night cycle, weather |
| [Navigation & Pathfinding](Navigation) | Flowfield BFS — thousands of bees, zero lag |
| [Architecture Overview](Architecture) | Code structure, modules, data flow |

---

## Two Views

The game has two camera perspectives toggled with **TAB**:

### Hive View (cross-section)
Inside the hive — comb cells, bees working, the queen at the centre.
Shows: cell contents, bee positions, colony stats (honey, pollen, temperature, brood).

### Meadow View (top-down)
Outside world — flowers, trees, pond, the hive building.
Shows: forage sources with fill-level indicators, forager bees in flight, weather info.

---

## Controls

| Key | Action |
|-----|--------|
| TAB | Switch between Hive View and Meadow View |
| P | Toggle pheromone debug overlay |
| LMB | Drop alarm pheromone at mouse position (debug) |
| S (held) | Drop trail pheromone at mouse position (debug) |
| ESC | Quit |
