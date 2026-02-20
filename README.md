# 🌲 Ironwood

A beginner-friendly, Scratch-inspired scripting language that reads like plain English — compiled to a single C++ file with no dependencies.

```
let name = ask "What's your name?"
say "Hello, " + name + "!"

let nums = [3, 1, 4, 1, 5, 9, 2, 6]
say sort nums
```

---

## Features

- **Plain-English syntax** — reads almost like pseudocode
- **Scratch-style operations** — `length of list`, `item 2 of list`, `keep items in list where fn`
- **Full language features** — functions, lambdas, classes, error handling, modules
- **String operations** — split, join, trim, replace, upper/lowercase, substring
- **File I/O** — read, write, append files
- **Networking** — HTTP fetch (GET/POST/PUT/etc.)
- **Subprocess** — run shell commands
- **JSON** — parse and serialize
- **Single-file interpreter** — one `.cpp` file, no external libraries

---

## Building

### Linux / macOS
```bash
g++ -std=c++17 -O2 -o ironwood ironwood_v2.cpp
```

### Windows (MinGW)
```bash
g++ -std=c++17 -O2 -o ironwood ironwood_v2.cpp -lws2_32
```

---

## Running a Program

```bash
./ironwood myprogram.irw
./ironwood myprogram.irw arg1 arg2    ; pass command-line arguments
```

---

## Quick Tour

### Output & Input
```
say "Hello, world!"
let name = ask "Enter your name: "
say "Hi, " + name
pause                                  ; wait for Enter
```

### Variables & Math
```
let x = 10
let y = 3.14
set x = x + 1
say x * y
say x % 3                             ; modulo
```

### Conditionals
```
if x > 5
  say "big"
else
  say "small"
end

; Inline ternary
let label = if x > 5 then "big" else "small"
```

### Loops
```
while x > 0
  say x
  set x = x - 1
end

for each item in [1, 2, 3]
  say item
end

; break and continue work inside loops
```

### Functions
```
function greet(name)
  return "Hello, " + name + "!"
end

say greet("World")

; Lambda (inline function)
let double = function(x)
  return x * 2
end
say double(5)
```

### Lists
```
let nums = [10, 20, 30]
add 40 to nums
say length of nums                    ; 4
say item 2 of nums                    ; 20  (1-indexed)

let evens = keep items in nums where function(n)
  return n % 2 == 0
end

say sort nums
say sort nums by function(n) return -n end   ; descending
```

### Dictionaries (Objects)
```
let person = {name: "Alice", age: 30}
say person.name
set person.age = 31

say has "name" in person             ; true
say keys of person
say values of person
```

### Strings
```
let s = "  Hello, World!  "
say trim s
say uppercase s
say lowercase s
say split s by ", "
say replace "World" with "Ironwood" in s
say index of "World" in s            ; 7 (0-based, -1 if not found)
say chars 1 to 5 of s                ; "Hello"
say length of s
```

### Classes
```
class Animal
  let name = "unknown"
  let sound = "..."

  function speak()
    say self.name + " says " + self.sound
  end
end

let cat = new Animal()
set cat.name = "Whiskers"
set cat.sound = "meow"
call cat.speak()
```

### Error Handling
```
try
  throw "something went wrong"
catch err
  say "Caught: " + err
end
```

### File I/O
```
write "Hello\nWorld" to file "output.txt"
append "\nLine 3" to file "output.txt"

let contents = read file "output.txt"
say contents

let lineList = lines of file "output.txt"
for each line in lineList
  say line
end

let exists = file exists "output.txt"
say exists                            ; true
```

### JSON
```
let data = {name: "Alice", scores: [95, 87, 92]}
let jsonStr = json of data
say jsonStr

let parsed = parse json jsonStr
say parsed.name
```

### Networking (HTTP)
```
let res = fetch "http://example.com/api/data"
if res.ok
  say res.body
end

; POST request
let res = fetch "http://example.com/api" with {
  method: "POST",
  body: json of {user: "alice"},
  headers: {Content-Type: "application/json"}
}
say res.status
```

### Subprocess
```
let result = run "echo hello"
if result.ok
  say result.output
end
say result.code                        ; exit code
```

### Modules
```
; get "stdlib" as std
let m = std.math
say m.sqrt(16)
say m.floor(3.9)
say m.random()

; Load a .irw file as a module
get "utils.irw" as utils
call utils.myFunction()
```

### Command-Line Arguments
```
; Access via the built-in `args` list
say length of args
say item 1 of args
```

### Misc
```
say type of 42          ; "number"
say type of "hello"     ; "string"
say type of []          ; "list"
say type of {}          ; "object"
say type of null        ; "null"
say type of true        ; "bool"

; Comments start with semicolons
; say "this won't run"
```

---

## Built-in Functions

| Function | Description |
|---|---|
| `say <expr>` | Print to stdout |
| `ask <prompt>` | Read a line from stdin |
| `pause` | Wait for Enter |
| `len(x)` | Length of string or list |
| `parseInt(x)` | Parse integer from string |
| `parseFloat(x)` | Parse float from string |
| `toString(x)` | Convert any value to string |
| `math.abs(n)` | Absolute value |
| `math.floor(n)` | Floor |
| `math.ceil(n)` | Ceiling |
| `math.sqrt(n)` | Square root |
| `math.pow(a,b)` | Power |
| `math.random()` | Random float 0–1 |

---

## Standard Library (`stdlib`)

```
get "stdlib" as std

std.math.sqrt(x)
std.math.pow(a, b)
std.math.random()
std.io.alert("message")
std.io.prompt("Enter: ")
std.io.confirm("Sure?")    ; returns true/false
```

---

## Language Cheatsheet

| Feature | Syntax |
|---|---|
| Variable | `let x = 5` |
| Reassign | `set x = 10` |
| Print | `say "hello"` |
| Input | `let x = ask "prompt"` |
| If/else | `if cond ... else ... end` |
| Ternary | `if cond then a else b` |
| While | `while cond ... end` |
| For each | `for each x in list ... end` |
| Function | `function name(a,b) ... end` |
| Lambda | `function(x) ... end` |
| Return | `return value` |
| Class | `class Name ... end` |
| New instance | `new ClassName()` |
| Self | `self.field` |
| Try/catch | `try ... catch err ... end` |
| Throw | `throw "message"` |
| Import | `get "module" as alias` |
| Comment | `; this is a comment` |

---

## Version History

| Version | Features |
|---|---|
| v1.0 | Variables, arithmetic, if/else, while, functions, lists |
| v1.1 | Scratch-style list ops (`length of`, `item N of`, `keep items where`) |
| v2.0 | Classes, try/catch/throw, dict ops (`has`, `keys`, `values`), file I/O |
| v3.0 | Lambdas, string ops, sort, ternary, JSON, command-line args, modules |
| v3.1 | HTTP fetch, subprocess (`run`), cross-platform (Linux/macOS/Windows) |

---

## License

MIT
