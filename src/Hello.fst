module Hello

// This is Hello in F* - it proves 1 + 1 = 2, and returns 42
val add_one : x:int -> Tot (y:int { y == x + 1 })
let add_one x = x + 1

val hello : unit -> Tot (x:int { x == 42 })
let hello () = 42
