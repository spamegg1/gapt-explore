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
val S4 = hos"P a, a = b :- P b"

// experiment with pattern matching
import gapt.expr.formula.fol.FOLFormula
val z = G match
  case _: Atom   => "atom"
  case And(a, b) => "and"
  case Or(a, b)  => "or"
  case Neg(a)    => "neg"
  case Imp(a, b) => "imp"
  case Iff(a, b) => "iff"
  case All(x, f) => "all"
  case Ex(x, f)  => "ex"

enum Same(x: FOLTerm, y: FOLTerm) extends FOLAtom:
  case SameRow(x: FOLTerm, y: FOLTerm) extends Same(x, y)
  case SameCol(x: FOLTerm, y: FOLTerm) extends Same(x, y)
  def alphaEquals(
      that: gapt.expr.Expr,
      lcBound: Int,
      thisCtx: Map[gapt.expr.Var, Int],
      thatCtx: Map[gapt.expr.Var, Int]
  ): Boolean = ???
  def alphaEquivalentHashCode: Int             = ???
  def syntaxEquals(e: gapt.expr.Expr): Boolean = ???
  def ty: gapt.expr.ty.Ty                      = ???
import Same.*

@main
def run: Unit =
  val F2 = fof"SameRow(x,c)"
  F2 match
    case FOLAtom("SameRow", FOLVar("x") :: FOLConst("c") :: Nil) => println("yay")
