// string interpolated formulas
val F  = fof"!x (P(x,f(x)) -> ?y P(x,y))"
val t  = fot"f(f(x))"
val G  = fof"!x (P(x,$t) -> ?y P(x,y))"
val H1 = hof"!x?y!z x(z) = y(y(z))"
val H2 = hof"∀x ∃y ∀z x(z) = y(y(z))"

// manually creating formulas
val x   = FOLVar("x")
val P   = Const("P", Ti ->: To)
val fx  = FOLFunction("f", x)
val Pfx = FOLAtom("P", fx)

val A = FOLAtom("A")
val B = FOLAtom("B")
val C = FOLAtom("C")
val D = (A & B) --> C

// Sequents
val S1 = Sequent()
val S2 = Sequent(List(1, 2), List(3, 4))
val S3 = Sequent(List(foa"A", foa"B"), List(foa"C", foa"D"))

// experiment with pattern matching
val z = G match
  case _: FOLAtom => ???
  case And(a, b)  => ???
  case Or(a, b)   => ???
  case Neg(a)     => ???
  case Imp(a, b)  => ???
  case Iff(a, b)  => ???
  case All(x, f)  => ???
  case Ex(x, f)   => ???

@main
def run: Unit =
  println(H2)
