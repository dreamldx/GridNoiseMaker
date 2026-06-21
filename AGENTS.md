# AGENTS.md - Development Rules

## Git Commit Convention

### Conventional Commits Specification

**Format:** `<type>(<scope>): <description>`

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, missing semicolons, etc.)
- `refactor`: Code refactoring (no functional changes)
- `perf`: Performance improvements
- `test`: Adding or updating tests
- `build`: Changes to build system or dependencies
- `ci`: CI/CD configuration changes
- `chore`: Other changes (maintenance, tooling, etc.)
- `revert`: Revert a previous commit

**Scopes:** Use descriptive scope (e.g., `feat(node-graph)`, `fix(app-shell)`, `docs(readme)`)

**Examples:**
```
feat(node-graph): add JSON serialization with type-based properties
fix(app-shell): correct menu item positioning on high-DPI displays
docs(openspec): update spec-driven workflow documentation
refactor(node-graph): extract ViewTransform into separate class
chore(deps): update nlohmann/json to v3.11.3
```

**Body:** Provide detailed explanation when needed, separated by blank line after subject.

**Breaking Changes:** Use `!` after type/scope and `BREAKING CHANGE:` in body:
```
feat(node-graph)!: change JSON schema to hierarchical structure

BREAKING CHANGE: Node properties now stored in nested "properties" object
```

## Verification Commands

Before marking work as complete:
1. Build application: `cd apps/imgui-shell && cmake --build --preset windows`
2. Run tests: [TODO: Add test command when available]
3. Check for lint issues: [TODO: Add lint command]

## OpenCode Skills

Always check for applicable skills before starting work. Use `/help` for skill guidance.