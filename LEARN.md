# 📘 Learning Ironwood — A Complete Guide

Ironwood is designed to be easy to read and write. If you've ever used Scratch, Python, or just written pseudocode, you'll feel right at home. This guide walks you through every feature from scratch (no pun intended), with examples and exercises at each step.

---

## Table of Contents

1. [Getting Started](#1-getting-started)
2. [Variables & Data Types](#2-variables--data-types)
3. [Math & Operators](#3-math--operators)
4. [Making Decisions](#4-making-decisions)
5. [Loops](#5-loops)
6. [Functions](#6-functions)
7. [Lists](#7-lists)
8. [Dictionaries](#8-dictionaries)
9. [Strings](#9-strings)
10. [Classes & Objects](#10-classes--objects)
11. [Error Handling](#11-error-handling)
12. [Files](#12-files)
13. [JSON](#13-json)
14. [Networking](#14-networking)
15. [Running Shell Commands](#15-running-shell-commands)
16. [Modules](#16-modules)
17. [Projects](#17-projects)

---

## 1. Getting Started

### Your first program

Create a file called `hello.irw` with this content:

```
say "Hello, world!"
```

Run it:
```bash
./ironwood hello.irw
```

You should see:
```
Hello, world!
```

### Comments

Lines starting with `;` are ignored by Ironwood. Use them to explain your code.

```
; This is a comment — Ironwood won't run this line
say "This runs"    ; you can also put comments at the end of a line
```

### Getting input from the user

```
let name = ask "What is your name? "
say "Nice to meet you, " + name + "!"
```

`ask` pauses and waits for the user to type something and press Enter. The result is stored in `name`.

### ✏️ Exercise 1
Write a program that asks the user for their favourite colour and then says: `"<colour> is a great colour!"`

---

## 2. Variables & Data Types

### Declaring variables

Use `let` to create a variable:

```
let age = 25
let name = "Alice"
let score = 9.5
let active = true
let nothing = null
```

### Changing a variable

Use `set` to change an existing variable:

```
let score = 0
set score = score + 10
say score    ; 10
```

You **cannot** use `let` to change a variable that already exists — that's what `set` is for.

### Data types

| Type | Example | Notes |
|---|---|---|
| Number | `42`, `3.14`, `-7` | All numbers are floating point |
| String | `"hello"` | Use double quotes |
| Boolean | `true`, `false` | |
| Null | `null` | Represents nothing/empty |
| List | `[1, 2, 3]` | See section 7 |
| Object | `{a: 1, b: 2}` | See section 8 |

### Checking types

```
say type of 42          ; "number"
say type of "hi"        ; "string"
say type of true        ; "bool"
say type of null        ; "null"
say type of [1,2]       ; "list"
say type of {a:1}       ; "object"
```

### ✏️ Exercise 2
Create variables of each type (number, string, boolean, null) and `say` each one. Then check their types using `type of`.

---

## 3. Math & Operators

### Arithmetic

```
say 10 + 3      ; 13
say 10 - 3      ; 7
say 10 * 3      ; 30
say 10 / 3      ; 3.333...
say 10 % 3      ; 1  (remainder)
```

### String joining

The `+` operator joins strings:

```
say "Hello" + ", " + "world!"
say "Score: " + 100
```

Numbers are automatically converted to strings when joined with `+`.

### Comparison operators

These return `true` or `false`:

```
say 5 == 5      ; true
say 5 != 3      ; true
say 5 > 3       ; true
say 5 < 3       ; false
say 5 >= 5      ; true
say 5 <= 4      ; false
```

### Logical operators

```
say true and false    ; false
say true or false     ; true
say not true          ; false
```

### Built-in math functions

```
say math.abs(-5)       ; 5
say math.sqrt(16)      ; 4
say math.floor(3.9)    ; 3
say math.ceil(3.1)     ; 4
say math.pow(2, 8)     ; 256
say math.random()      ; random number between 0 and 1
```

### ✏️ Exercise 3
Write a program that asks for two numbers and prints their sum, difference, product, and remainder.

---

## 4. Making Decisions

### if / else

```
let score = 75

if score >= 90
  say "A grade"
else
  if score >= 70
    say "B grade"
  else
    if score >= 50
      say "C grade"
    else
      say "Fail"
    end
  end
end
```

Every `if` block ends with `end`. To chain multiple conditions, nest `if` inside `else` — each one needs its own `end`.

### Inline ternary (if-then-else expression)

For simple choices in a single line:

```
let age = 20
let status = if age >= 18 then "adult" else "minor"
say status
```

This is an *expression*, so it produces a value you can store or use directly.

### Truthiness

In Ironwood, these values are considered **false**: `false`, `null`, `0`, `""` (empty string).  
Everything else is **true**.

```
let name = ""
if name
  say "Got a name"
else
  say "Name is empty"     ; this runs
end
```

### ✏️ Exercise 4
Ask the user for a number. If it's even (divisible by 2), say "even". Otherwise say "odd". Tip: use `%`.

---

## 5. Loops

### while loop

Repeats as long as a condition is true:

```
let count = 1
while count <= 5
  say count
  set count = count + 1
end
```

### for each loop

Loops over every item in a list:

```
let fruits = ["apple", "banana", "cherry"]
for each fruit in fruits
  say fruit
end
```

You can also loop over a range by building a list, or nest loops:

```
for each i in [1, 2, 3]
  for each j in [1, 2, 3]
    say i + " x " + j + " = " + i * j
  end
end
```

### break and continue

```
let i = 0
while true
  set i = i + 1
  if i % 2 == 0
    continue      ; skip even numbers
  end
  if i > 10
    break         ; stop the loop
  end
  say i
end
```

### ✏️ Exercise 5
Write a program that prints the multiplication table for a number the user enters (from 1×1 to 1×12).

---

## 6. Functions

### Defining and calling a function

```
function greet(name)
  say "Hello, " + name + "!"
end

call greet("Alice")
call greet("Bob")
```

### Returning values

```
function add(a, b)
  return a + b
end

let result = add(3, 4)
say result       ; 7
```

### Functions are values (lambdas)

You can store a function in a variable and pass it around:

```
let square = function(x)
  return x * x
end

say square(5)     ; 25

; Pass a function to another function
function applyTwice(fn, value)
  return fn(fn(value))
end

say applyTwice(square, 2)    ; 16
```

### Recursion

Functions can call themselves:

```
function factorial(n)
  if n <= 1
    return 1
  end
  return n * factorial(n - 1)
end

say factorial(5)    ; 120
```

### ✏️ Exercise 6
Write a function `isPrime(n)` that returns `true` if `n` is a prime number, `false` otherwise. Test it on several numbers.

---

## 7. Lists

Lists hold multiple values in order.

### Creating and accessing lists

```
let colors = ["red", "green", "blue"]

say length of colors          ; 3
say item 1 of colors          ; "red"  (1-indexed!)
say item 3 of colors          ; "blue"
```

> **Note:** Ironwood lists use **1-based indexing** — the first item is item 1, not item 0.

### Adding items

```
let nums = [1, 2, 3]
add 4 to nums
say nums                      ; [1,2,3,4]
```

### Looping over a list

```
for each color in colors
  say color
end
```

### Filtering with keep

```
let nums = [1, 2, 3, 4, 5, 6, 7, 8]

let evens = keep items in nums where function(n)
  return n % 2 == 0
end

say evens       ; [2,4,6,8]
```

### Sorting

```
let nums = [3, 1, 4, 1, 5, 9, 2]
say sort nums                  ; [1,1,2,3,4,5,9]

; Sort descending
say sort nums by function(n) return -n end

; Sort objects by a field
let people = [{name:"Charlie",age:30},{name:"Alice",age:25},{name:"Bob",age:35}]
say sort people by age
say sort people by name
```

### ✏️ Exercise 7
Write a program that:
1. Starts with a list of numbers
2. Filters out all numbers less than 10
3. Sorts the remaining numbers
4. Prints each one on its own line

---

## 8. Dictionaries

Dictionaries (also called objects or maps) store key-value pairs.

### Creating and accessing

```
let person = {name: "Alice", age: 30, active: true}

say person.name       ; "Alice"
say person.age        ; 30
```

### Changing and adding fields

```
set person.age = 31
set person.email = "alice@example.com"
say person
```

### Checking for keys

```
if has "email" in person
  say "Has email: " + person.email
end
```

### Getting all keys or values

```
say keys of person        ; list of key names
say values of person      ; list of values
```

### Nested structures

```
let config = {
  server: {host: "localhost", port: 8080},
  debug: true
}

say config.server.host
say config.server.port
```

### ✏️ Exercise 8
Create a dictionary representing a book (title, author, year, pages). Print a formatted summary like: `"Title by Author (Year) — N pages"`.

---

## 9. Strings

### Escape characters

```
say "Line 1\nLine 2"      ; \n = newline
say "Tab\there"           ; \t = tab
say "Quote: \"hello\""    ; \" = literal quote
```

### String operations

```
let s = "  Hello, World!  "

say trim s                             ; "Hello, World!"
say uppercase s                        ; "  HELLO, WORLD!  "
say lowercase s                        ; "  hello, world!  "
say replace "World" with "Ironwood" in s
say index of "World" in s              ; 8  (-1 if not found)
say chars 3 to 7 of s                  ; "Hello"
say length of s                        ; 17
```

### Splitting and joining

```
let csv = "alice,bob,charlie"
let names = split csv by ","
say names                   ; ["alice","bob","charlie"]

let joined = join names with " | "
say joined                  ; "alice | bob | charlie"
```

### Building strings

```
let name = "Alice"
let score = 99
say "Player " + name + " scored " + score + " points!"
```

### ✏️ Exercise 9
Ask the user to type a sentence. Then:
1. Count the words (split by space)
2. Print the sentence in uppercase
3. Print the sentence reversed word by word

---

## 10. Classes & Objects

Classes let you bundle data and behaviour together.

### Defining a class

```
class Dog
  let name = "unnamed"
  let breed = "unknown"
  let tricks = []

  function bark()
    say self.name + " says: Woof!"
  end

  function learnTrick(trick)
    add trick to self.tricks
    say self.name + " learned " + trick + "!"
  end

  function showTricks()
    say self.name + " knows:"
    for each trick in self.tricks
      say "  - " + trick
    end
  end
end
```

### Creating instances

```
let rex = new Dog()
set rex.name = "Rex"
set rex.breed = "Labrador"

call rex.bark()
call rex.learnTrick("sit")
call rex.learnTrick("shake")
call rex.showTricks()
```

### Self

Inside a method, `self` refers to the current object. Use `self.field` to access or change the object's own data.

### Multiple instances

Each instance is independent:

```
let spot = new Dog()
set spot.name = "Spot"
call spot.learnTrick("roll over")

call rex.showTricks()    ; Rex's tricks (sit, shake)
call spot.showTricks()   ; Spot's tricks (roll over)
```

### ✏️ Exercise 10
Create a `BankAccount` class with:
- A `balance` field starting at 0
- A `deposit(amount)` method that adds to the balance and prints the new balance
- A `withdraw(amount)` method that subtracts from the balance (but refuses if there isn't enough money)
- A `getBalance()` method that returns the balance

---

## 11. Error Handling

### try / catch

Wrap risky code in `try`. If something goes wrong, the `catch` block runs:

```
try
  let data = read file "missing.txt"
  say data
catch err
  say "Error: " + err
end
```

### throw

You can throw your own errors:

```
function divide(a, b)
  if b == 0
    throw "Cannot divide by zero!"
  end
  return a / b
end

try
  say divide(10, 0)
catch err
  say "Caught: " + err
end
```

### Nested try/catch

```
try
  try
    throw "inner error"
  catch e
    say "Inner caught: " + e
    throw "outer error"    ; re-throw or throw a new one
  end
catch e
  say "Outer caught: " + e
end
```

### ✏️ Exercise 11
Write a function `safeDivide(a, b)` that returns the result of `a / b`, or returns `null` and prints a warning if `b` is zero. Use try/catch inside.

---

## 12. Files

### Writing a file

```
write "Hello, file!" to file "output.txt"
```

This **overwrites** the file if it already exists.

### Appending to a file

```
append "\nSecond line" to file "output.txt"
append "\nThird line" to file "output.txt"
```

### Reading a file

```
let contents = read file "output.txt"
say contents
```

### Reading line by line

```
let lines = lines of file "output.txt"
say "Total lines: " + length of lines

for each line in lines
  say "> " + line
end
```

### Checking if a file exists

```
if file exists "output.txt"
  say "File found!"
else
  say "File not found."
end
```

### Full example: simple log

```
function logMessage(msg)
  append msg + "\n" to file "log.txt"
end

call logMessage("Program started")
call logMessage("Doing some work...")
call logMessage("Program finished")

say "Log contents:"
say read file "log.txt"
```

### ✏️ Exercise 12
Write a program that:
1. Asks the user for their name
2. Writes a welcome message to `welcome.txt`
3. Reads it back and prints it
4. Appends "Goodbye!" to the file

---

## 13. JSON

JSON is a standard text format for data. Ironwood can convert between its values and JSON strings.

### Converting to JSON

```
let data = {
  name: "Alice",
  scores: [95, 87, 92],
  active: true
}

let jsonStr = json of data
say jsonStr
; {"name":"Alice","scores":[95,87,92],"active":true}
```

### Parsing JSON

```
let jsonStr = "{\"city\":\"London\",\"population\":9000000}"
let city = parse json jsonStr
say city.name      ; London
say city.population
```

### Saving structured data to a file

```
let records = [
  {id: 1, name: "Alice", score: 95},
  {id: 2, name: "Bob",   score: 87}
]

write json of records to file "records.json"

; Load it back
let loaded = parse json read file "records.json"
for each r in loaded
  say r.name + ": " + r.score
end
```

### ✏️ Exercise 13
Write a simple to-do list that saves tasks to a `todos.json` file:
1. Load existing tasks from the file (if it exists)
2. Ask the user for a new task
3. Add it to the list
4. Save the updated list back to the file
5. Print all tasks

---

## 14. Networking

Ironwood can make HTTP requests using the `fetch` keyword.

> **Note:** Only plain `http://` URLs are supported (not `https://`). For HTTPS, a tool like a local proxy can be used.

### Simple GET request

```
let response = fetch "http://httpbin.org/get"

if response.ok
  say "Status: " + response.status
  say response.body
else
  say "Request failed with status: " + response.status
end
```

The response object has three fields:
- `body` — the response text
- `status` — the HTTP status code (200, 404, etc.)
- `ok` — true if status is 200–299

### POST request

```
let response = fetch "http://httpbin.org/post" with {
  method: "POST",
  body: "name=Alice&score=99"
}

say response.body
```

### POST with JSON

```
let payload = json of {username: "alice", password: "secret"}

let response = fetch "http://myapi.example.com/login" with {
  method: "POST",
  body: payload,
  headers: {Content-Type: "application/json"}
}

if response.ok
  let data = parse json response.body
  say "Logged in! Token: " + data.token
else
  say "Login failed: " + response.status
end
```

### ✏️ Exercise 14
Fetch `http://httpbin.org/uuid` (it returns a random UUID in JSON). Parse the response and print just the UUID value.

---

## 15. Running Shell Commands

Use `run` to execute any shell command.

```
let result = run "echo Hello from the shell"

if result.ok
  say result.output
end

say "Exit code: " + result.code
```

The result object has:
- `output` — all stdout and stderr combined as a string
- `code` — the exit code (0 = success)
- `ok` — true if exit code is 0

### Practical examples

```
; List files
let ls = run "ls -la"
say ls.output

; Get current directory
let pwd = run "pwd"
say trim pwd.output

; Check if a tool is installed
let which = run "which python3"
if which.ok
  say "Python 3 is installed at: " + trim which.output
else
  say "Python 3 is not installed"
end
```

### ✏️ Exercise 15
Write a program that runs `ping -c 4 google.com` (Linux/Mac) or `ping -n 4 google.com` (Windows), captures the output, and prints whether the ping succeeded.

---

## 16. Modules

You can split your code into multiple `.irw` files and import them.

### Creating a module (mathutils.irw)

```
function clamp(val, minVal, maxVal)
  if val < minVal
    return minVal
  end
  if val > maxVal
    return maxVal
  end
  return val
end

function lerp(a, b, t)
  return a + (b - a) * t
end

function sign(n)
  return if n > 0 then 1 else if n < 0 then -1 else 0
end
```

### Importing the module

```
get "mathutils.irw" as mu

say mu.clamp(150, 0, 100)     ; 100
say mu.lerp(0, 100, 0.25)     ; 25
say mu.sign(-42)               ; -1
```

### The built-in stdlib

```
get "stdlib" as std

say std.math.sqrt(49)          ; 7
say std.math.random()          ; 0.something

let answer = std.io.confirm("Are you sure?")
if answer
  say "Confirmed!"
end
```

### ✏️ Exercise 16
Create a module `stringutils.irw` with functions:
- `isPalindrome(s)` — returns true if the string reads the same forwards and backwards
- `countChar(s, ch)` — counts how many times character `ch` appears in `s`
- `repeat(s, n)` — returns the string repeated n times

Then import and test it from a main program.

---

## 17. Projects

Now that you know everything, here are some complete projects to try.

---

### Project 1: Number Guessing Game

```
let secret = parseInt(math.random() * 100 + 1)
let attempts = 0
let won = false

say "I'm thinking of a number between 1 and 100."

while not won
  let guess = parseInt(ask "Your guess: ")
  set attempts = attempts + 1

  if guess == secret
    set won = true
    say "Correct! You got it in " + attempts + " attempts!"
  else
    if guess < secret
      say "Too low!"
    else
      say "Too high!"
    end
  end
end
```

---

### Project 2: To-Do List (with persistence)

```
let filename = "todos.json"
let todos = []

; Load existing todos
if file exists filename
  set todos = parse json read file filename
end

; Main loop
let running = true
while running
  say ""
  say "=== To-Do List ==="
  if length of todos == 0
    say "(empty)"
  end
  let i = 1
  for each todo in todos
    let status = if todo.done then "[x]" else "[ ]"
    say status + " " + i + ". " + todo.text
    set i = i + 1
  end

  say ""
  say "1) Add  2) Complete  3) Delete  4) Quit"
  let choice = ask "> "

  if choice == "1"
    let text = ask "New task: "
    add {text: text, done: false} to todos
    write json of todos to file filename
    say "Added!"
  else
    if choice == "2"
      let idx = parseInt(ask "Task number: ") - 1
      set todos[idx].done = true
      write json of todos to file filename
      say "Marked done!"
    else
      if choice == "3"
        let idx = parseInt(ask "Task number: ")
        set todos = keep items in todos where function(t)
          set idx = idx - 1
          return idx != 0
        end
        write json of todos to file filename
        say "Deleted!"
      else
        if choice == "4"
          set running = false
          say "Goodbye!"
        end
      end
    end
  end
end
```

---

### Project 3: Word Frequency Counter

```
let filename = ask "Enter a filename: "

if not file exists filename
  say "File not found: " + filename
else
  let text = lowercase read file filename
  let words = split text by " "

  let freq = {}
  for each word in words
    set word = trim word
    if length of word > 0
      if has word in freq
        set freq[word] = freq[word] + 1
      else
        set freq[word] = 1
      end
    end
  end

  say "Word frequencies:"
  let wordList = keys of freq
  let sorted = sort wordList by function(w) return -freq[w] end

  let i = 0
  for each w in sorted
    if i < 20
      say w + ": " + freq[w]
      set i = i + 1
    end
  end
end
```

---

### Project 4: Mini Calculator

```
say "Mini Calculator (type 'quit' to exit)"

let running = true
while running
  let input = ask "> "

  if input == "quit"
    set running = false

  else
    ; Try to parse as: number op number
    try
      let parts = split input by " "
      let a = parseFloat(item 1 of parts)
      let op = item 2 of parts
      let b = parseFloat(item 3 of parts)

      if op == "+"
        say a + b
      else
        if op == "-"
          say a - b
        else
          if op == "*"
            say a * b
          else
            if op == "/"
              if b == 0
                say "Error: division by zero"
              else
                say a / b
              end
            else
              if op == "%"
                say a % b
              else
                say "Unknown operator: " + op
              end
            end
          end
        end
      end

    catch err
      say "Could not parse. Try: 5 + 3"
    end
  end
end
```

---

## Quick Reference Card

```
; Variables
let x = 10
set x = x + 1

; Output / Input
say "hello"
let name = ask "Name? "
pause

; Types
type of x          ; "number", "string", "bool", "null", "list", "object"

; Math
+  -  *  /  %
math.sqrt(x)   math.floor(x)   math.ceil(x)
math.pow(a,b)  math.abs(x)     math.random()

; Logic
==  !=  <  >  <=  >=
and  or  not

; Conditionals
if cond ... else ... end
if cond then a else b        ; ternary
; chain conditions by nesting if inside else:
; if c1 ... else  if c2 ... else ... end  end

; Loops
while cond ... end
for each x in list ... end
break  /  continue

; Functions
function name(a, b) ... end
let fn = function(x) ... end
return value

; Lists
let a = [1, 2, 3]
add x to a
length of a
item N of a                  ; 1-indexed
sort a
sort a by fn
keep items in a where fn

; Dicts
let d = {k: v}
d.key
has "key" in d
keys of d
values of d

; Strings
length of s
trim s    uppercase s    lowercase s
split s by sep
join list with sep
replace x with y in s
index of sub in s
chars i to j of s

; Classes
class Name ... end
new ClassName()
self.field

; Errors
try ... catch err ... end
throw "message"

; Files
write str to file path
append str to file path
read file path
lines of file path
file exists path

; JSON
json of value
parse json str

; Network
fetch "url"
fetch "url" with {method, body, headers}
; response: {body, status, ok}

; Shell
run "command"
; result: {output, code, ok}

; Modules
get "stdlib" as std
get "myfile.irw" as mod
```

---

*Happy coding with Ironwood! 🌲*
