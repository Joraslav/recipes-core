# Recipes

Recipes is a C++23 application for storing kitchen stock and recipes in SQLite and exporting the result as:

- console output
- JSON
- YAML

It can show either all recipes or only recipes that are currently cookable with the available stock.

## Features

- SQLite-backed storage for products and recipes
- cookable recipe selection based on the current inventory
- console, JSON, and YAML reports
- configurable database and output paths
- GoogleTest test suite

## Repository layout

| Path | Purpose |
|---|---|
| `src/app` | Application orchestration |
| `src/db` | SQLite access layer |
| `src/io` | CLI parsing and report dispatch |
| `src/io/console` | Human-readable console reports |
| `src/io/json` | JSON export |
| `src/io/yaml` | YAML export |
| `src/concepts` | Shared C++ concepts |
| `src/unites` | Strongly typed unit helpers |
| `src/types` | Domain types and SQL schema |
| `src/main.cpp` | Application entry point |
| `tests` | GoogleTest-based tests |
| `scripts` | Helper scripts for setup and testing |

## Requirements

- CMake 4.1 or newer
- Conan 2
- A C++23-capable compiler
- `ctest` for running the test suite
- Bash for the helper scripts

On Fedora, `scripts/create_env.sh` also expects Perl and a few core Perl modules.

## Build

### Recommended: use the helper scripts

1. Install dependencies:

   ```bash
   ./scripts/create_env.sh Debug
   ```

   Use `Release` instead of `Debug` if you want a release build environment.

2. Configure the project:

   ```bash
   cmake --preset conan-debug
   ```

3. Build it:

   ```bash
   cmake --build --preset conan-debug
   ```

### Manual CMake/Conan flow

If you prefer to run the commands yourself, the same flow is:

```bash
conan install . --build=missing -pr:h profiles/Debug -pr:b profiles/Release
cmake --preset conan-debug
cmake --build --preset conan-debug
```

On Fedora, use the matching `profiles/Fedora-Debug` and `profiles/Fedora-Release` files instead.

## Run tests

The easiest way is:

```bash
./scripts/run_tests.sh --build-type Debug
```

Useful options:

- `--build-type Release` — run the Release test configuration
- `--build-dir build/Debug` — use a custom build directory
- `--jobs 4` — limit or increase parallelism

If you want to do it manually:

```bash
ctest --test-dir build/Debug --build-config Debug --output-on-failure --parallel 4
```

## Run the application

After building, run the `Recipes` executable from your build directory:

```bash
./build/Debug/Recipes --help
```

## Run the HTTP/HTTPS server

The server starts when the `--server-config` option is provided. The
configuration format is selected by the file extension and can be JSON, YAML
or YML.

Start the HTTP listener using the sample configuration:

```bash
./build/Debug/Recipes --server-config=config/server.json
```

The process prints the bound HTTP port and stays alive until it receives
`SIGINT` or `SIGTERM`. Stop a foreground process with `Ctrl+C`.

To enable HTTPS, provide certificate and private-key files and use:

```bash
./build/Debug/Recipes --server-config=config/server-https.json
```

The HTTPS listener performs a TLS server handshake before passing the
connection to the same Beast session and REST router used by HTTP. The
certificate paths are relative to the process working directory.

The application reads from a SQLite database. By default, it expects:

```text
out/data/table.db
```

and writes reports to:

```text
out/info.json
out/info.yaml
```

If the database is empty, the reports will be empty too. Populate it through the library API or point the app at an existing SQLite database.

## CLI options

| Option | Meaning |
|---|---|
| `-h`, `--help` | Show help |
| `--db-path <path>` | Use a custom SQLite database |
| `--all-recipes` | Query all recipes |
| `--cookable` | Query only cookable recipes |
| `--full` | Print full recipe details in the console |
| `--short` | Print only recipe names and ingredient counts |
| `--console` / `--no-console` | Enable or disable console output |
| `--json-out <path>` / `--no-json` | Control JSON output |
| `--yaml-out <path>` / `--no-yaml` | Control YAML output |

By default, console, JSON, and YAML output are all enabled.

## Examples

Show help:

```bash
./build/Debug/Recipes --help
```

Query only cookable recipes and print a short console summary:

```bash
./build/Debug/Recipes --cookable --short --no-json --no-yaml
```

Write reports to custom files:

```bash
./build/Debug/Recipes \
  --db-path ./data/recipes.db \
  --json-out ./artifacts/recipes.json \
  --yaml-out ./artifacts/recipes.yaml \
  --no-console
```

Use the full console report:

```bash
./build/Debug/Recipes --all-recipes --full
```

## Example outputs

### Console

```text
Name: Pancake
Ingredients count: 2
```

### JSON

```json
[
  {
    "name": "Tea",
    "ingredients": [
      { "name": "Water", "amount": 200, "dimension": "ml" },
      { "name": "Tea leaf", "amount": 5, "dimension": "gr" }
    ]
  }
]
```

### YAML

```yaml
- name: Tea
  ingredients:
    - name: Water
      amount: 200
      dimension: ml
    - name: Tea leaf
      amount: 5
      dimension: gr
```

## Library usage

The core pieces are also usable from C++ code:

```cpp
#include "app/App.hpp"
#include "db/DBManager.hpp"
#include "io/Args.hpp"
#include "types/kitchen/Types.hpp"

db::DBManager db;
db.InsertProduct(types::Product{"Milk", 200, types::Dimension::Milliliter});
db.InsertRecipe(types::Recipe{"Pancake", {
    types::Product{"Milk", 100, types::Dimension::Milliliter},
}});

io::Args args;
args.AppMutable().recipe_selection = io::RecipeSelection::Cookable;
args.OutMutable().write_json = false;
args.OutMutable().write_yaml = false;
app::Run(args);
```

## Notes

- The default database path and report paths are relative to the project root.
- The database schema is created automatically when the app opens the database.
- The application uses SQLite, SQLiteCpp, and Glaze for persistence and serialization.
