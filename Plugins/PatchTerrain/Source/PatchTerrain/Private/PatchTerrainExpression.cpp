#include "PatchTerrainExpression.h"

#include "PatchTerrainNoise.h"

namespace PatchTerrainExpr
{
	enum class EFunc : uint8
	{
		Sin, Cos, Tan, Asin, Acos, Atan, Atan2, Abs, Sqrt, Exp, Log, Pow,
		Floor, Ceil, Round, Frac, Sign, Min, Max, Mod,
		Clamp, Lerp, SmoothStep, Step,
		Noise, Fbm, Ridged, Value, ValueFbm,
	};

	struct FFuncDesc
	{
		const TCHAR* Name;
		EFunc Func;
		int32 Argc;
	};

	static const FFuncDesc Functions[] =
	{
		{ TEXT("sin"), EFunc::Sin, 1 },
		{ TEXT("cos"), EFunc::Cos, 1 },
		{ TEXT("tan"), EFunc::Tan, 1 },
		{ TEXT("asin"), EFunc::Asin, 1 },
		{ TEXT("acos"), EFunc::Acos, 1 },
		{ TEXT("atan"), EFunc::Atan, 1 },
		{ TEXT("atan2"), EFunc::Atan2, 2 },
		{ TEXT("abs"), EFunc::Abs, 1 },
		{ TEXT("sqrt"), EFunc::Sqrt, 1 },
		{ TEXT("exp"), EFunc::Exp, 1 },
		{ TEXT("log"), EFunc::Log, 1 },
		{ TEXT("pow"), EFunc::Pow, 2 },
		{ TEXT("floor"), EFunc::Floor, 1 },
		{ TEXT("ceil"), EFunc::Ceil, 1 },
		{ TEXT("round"), EFunc::Round, 1 },
		{ TEXT("frac"), EFunc::Frac, 1 },
		{ TEXT("sign"), EFunc::Sign, 1 },
		{ TEXT("min"), EFunc::Min, 2 },
		{ TEXT("max"), EFunc::Max, 2 },
		{ TEXT("mod"), EFunc::Mod, 2 },
		{ TEXT("clamp"), EFunc::Clamp, 3 },
		{ TEXT("lerp"), EFunc::Lerp, 3 },
		{ TEXT("smoothstep"), EFunc::SmoothStep, 3 },
		{ TEXT("step"), EFunc::Step, 2 },
		{ TEXT("noise"), EFunc::Noise, 2 },
		{ TEXT("fbm"), EFunc::Fbm, 3 },
		{ TEXT("ridged"), EFunc::Ridged, 3 },
		{ TEXT("value"), EFunc::Value, 2 },
		{ TEXT("vfbm"), EFunc::ValueFbm, 3 },
	};

	static const FFuncDesc* FindFunction(const FString& Name)
	{
		for (const FFuncDesc& Desc : Functions)
		{
			if (Name.Equals(Desc.Name, ESearchCase::IgnoreCase))
			{
				return &Desc;
			}
		}
		return nullptr;
	}

	/** Modulo toujours positif pour un diviseur positif (plus naturel pour des motifs répétés). */
	static double PositiveMod(double A, double B)
	{
		if (B == 0.0)
		{
			return 0.0;
		}
		const double R = FMath::Fmod(A, B);
		return (R != 0.0 && ((R < 0.0) != (B < 0.0))) ? R + B : R;
	}

	enum class ETok : uint8 { Number, Ident, Op, LParen, RParen, Comma, End };

	struct FToken
	{
		ETok Type = ETok::End;
		FString Text;
		double Number = 0.0;
		int32 Pos = 0;
	};

	/** Descente récursive qui émet directement le code de la machine à pile. */
	class FParser
	{
	public:
		using FInstr = FPatchTerrainExpression::FInstr;
		using EOp = FPatchTerrainExpression::EOp;

		FParser(const FString& InSource, TArray<FInstr>& InCode)
			: Source(InSource), Code(InCode)
		{
		}

		bool Run(FString& OutError)
		{
			if (!Tokenize())
			{
				OutError = Error;
				return false;
			}
			if (Tokens.Num() == 1)
			{
				OutError = TEXT("Expression vide.");
				return false;
			}
			if (!ParseComparison())
			{
				OutError = Error;
				return false;
			}
			if (Peek().Type != ETok::End)
			{
				OutError = FString::Printf(TEXT("Symbole inattendu '%s' (position %d)."), *Peek().Text, Peek().Pos + 1);
				return false;
			}
			if (MaxDepth > FPatchTerrainExpression::MaxStackDepth)
			{
				OutError = TEXT("Expression trop imbriquée.");
				return false;
			}
			return true;
		}

	private:
		bool Tokenize()
		{
			int32 I = 0;
			const int32 Len = Source.Len();
			while (I < Len)
			{
				const TCHAR C = Source[I];
				if (FChar::IsWhitespace(C))
				{
					++I;
					continue;
				}

				FToken Tok;
				Tok.Pos = I;

				if (FChar::IsDigit(C) || (C == TEXT('.') && I + 1 < Len && FChar::IsDigit(Source[I + 1])))
				{
					int32 J = I;
					while (J < Len && (FChar::IsDigit(Source[J]) || Source[J] == TEXT('.')))
					{
						++J;
					}
					// Exposant : 1e3, 2.5E-2
					if (J < Len && (Source[J] == TEXT('e') || Source[J] == TEXT('E')))
					{
						int32 K = J + 1;
						if (K < Len && (Source[K] == TEXT('+') || Source[K] == TEXT('-')))
						{
							++K;
						}
						if (K < Len && FChar::IsDigit(Source[K]))
						{
							J = K;
							while (J < Len && FChar::IsDigit(Source[J]))
							{
								++J;
							}
						}
					}
					Tok.Type = ETok::Number;
					Tok.Text = Source.Mid(I, J - I);
					Tok.Number = FCString::Atod(*Tok.Text);
					I = J;
				}
				else if (FChar::IsAlpha(C) || C == TEXT('_'))
				{
					int32 J = I;
					while (J < Len && (FChar::IsAlnum(Source[J]) || Source[J] == TEXT('_')))
					{
						++J;
					}
					Tok.Type = ETok::Ident;
					Tok.Text = Source.Mid(I, J - I).ToLower();
					I = J;
				}
				else if (C == TEXT('('))
				{
					Tok.Type = ETok::LParen; Tok.Text = TEXT("("); ++I;
				}
				else if (C == TEXT(')'))
				{
					Tok.Type = ETok::RParen; Tok.Text = TEXT(")"); ++I;
				}
				else if (C == TEXT(','))
				{
					Tok.Type = ETok::Comma; Tok.Text = TEXT(","); ++I;
				}
				else if (FCString::Strchr(TEXT("+-*/%^<>"), C))
				{
					Tok.Type = ETok::Op; Tok.Text = FString::Chr(C); ++I;
				}
				else
				{
					Error = FString::Printf(TEXT("Caractère inconnu '%c' (position %d)."), C, I + 1);
					return false;
				}
				Tokens.Add(Tok);
			}

			FToken EndTok;
			EndTok.Type = ETok::End;
			EndTok.Text = TEXT("fin");
			EndTok.Pos = Len;
			Tokens.Add(EndTok);
			return true;
		}

		const FToken& Peek() const { return Tokens[Index]; }
		const FToken& Next() { return Tokens[Index++]; }

		bool IsOp(const TCHAR* Op) const
		{
			return Peek().Type == ETok::Op && Peek().Text == Op;
		}

		/** Émet une instruction et suit la profondeur de pile (Delta = variation nette). */
		void Emit(EOp Op, int32 Delta, double Value = 0.0, uint8 Func = 0)
		{
			FInstr Instr;
			Instr.Op = Op;
			Instr.Value = Value;
			Instr.Func = Func;
			Code.Add(Instr);
			Depth += Delta;
			MaxDepth = FMath::Max(MaxDepth, Depth);
		}

		bool Fail(const FString& Message)
		{
			if (Error.IsEmpty())
			{
				Error = FString::Printf(TEXT("%s (position %d)."), *Message, Peek().Pos + 1);
			}
			return false;
		}

		// comparaison := additif (('<' | '>') additif)?
		bool ParseComparison()
		{
			if (!ParseAdditive())
			{
				return false;
			}
			if (IsOp(TEXT("<")) || IsOp(TEXT(">")))
			{
				const bool bLess = Next().Text == TEXT("<");
				if (!ParseAdditive())
				{
					return false;
				}
				Emit(bLess ? EOp::Less : EOp::Greater, -1);
			}
			return true;
		}

		// additif := terme (('+' | '-') terme)*
		bool ParseAdditive()
		{
			if (!ParseTerm())
			{
				return false;
			}
			while (IsOp(TEXT("+")) || IsOp(TEXT("-")))
			{
				const bool bAdd = Next().Text == TEXT("+");
				if (!ParseTerm())
				{
					return false;
				}
				Emit(bAdd ? EOp::Add : EOp::Sub, -1);
			}
			return true;
		}

		// terme := unaire (('*' | '/' | '%') unaire)*
		bool ParseTerm()
		{
			if (!ParseUnary())
			{
				return false;
			}
			while (IsOp(TEXT("*")) || IsOp(TEXT("/")) || IsOp(TEXT("%")))
			{
				const FString Op = Next().Text;
				if (!ParseUnary())
				{
					return false;
				}
				Emit(Op == TEXT("*") ? EOp::Mul : (Op == TEXT("/") ? EOp::Div : EOp::Mod), -1);
			}
			return true;
		}

		// unaire := ('-' | '+') unaire | puissance      (donc -2^2 = -(2^2), comme en maths)
		bool ParseUnary()
		{
			if (IsOp(TEXT("-")))
			{
				Next();
				if (!ParseUnary())
				{
					return false;
				}
				Emit(EOp::Neg, 0);
				return true;
			}
			if (IsOp(TEXT("+")))
			{
				Next();
				return ParseUnary();
			}
			return ParsePower();
		}

		// puissance := primaire ('^' unaire)?          (associative à droite : 2^3^2 = 2^9)
		bool ParsePower()
		{
			if (!ParsePrimary())
			{
				return false;
			}
			if (IsOp(TEXT("^")))
			{
				Next();
				if (!ParseUnary())
				{
					return false;
				}
				Emit(EOp::Pow, -1);
			}
			return true;
		}

		// primaire := nombre | variable | constante | fonction '(' args ')' | '(' expression ')'
		bool ParsePrimary()
		{
			const FToken Tok = Peek();

			if (Tok.Type == ETok::Number)
			{
				Next();
				Emit(EOp::Const, +1, Tok.Number);
				return true;
			}

			if (Tok.Type == ETok::LParen)
			{
				Next();
				if (!ParseComparison())
				{
					return false;
				}
				if (Peek().Type != ETok::RParen)
				{
					return Fail(TEXT("Parenthèse fermante attendue"));
				}
				Next();
				return true;
			}

			if (Tok.Type == ETok::Ident)
			{
				Next();

				// Appel de fonction
				if (Peek().Type == ETok::LParen)
				{
					const FFuncDesc* Desc = FindFunction(Tok.Text);
					if (!Desc)
					{
						return Fail(FString::Printf(TEXT("Fonction inconnue '%s'"), *Tok.Text));
					}
					Next();

					int32 Argc = 0;
					if (Peek().Type != ETok::RParen)
					{
						for (;;)
						{
							if (!ParseComparison())
							{
								return false;
							}
							++Argc;
							if (Peek().Type == ETok::Comma)
							{
								Next();
								continue;
							}
							break;
						}
					}
					if (Peek().Type != ETok::RParen)
					{
						return Fail(TEXT("Parenthèse fermante attendue"));
					}
					Next();

					if (Argc != Desc->Argc)
					{
						return Fail(FString::Printf(TEXT("'%s' attend %d argument(s), pas %d"), Desc->Name, Desc->Argc, Argc));
					}
					Emit(EOp::Call, 1 - Argc, 0.0, static_cast<uint8>(Desc->Func));
					return true;
				}

				// Variables et constantes
				if (Tok.Text == TEXT("x")) { Emit(EOp::VarX, +1); return true; }
				if (Tok.Text == TEXT("y")) { Emit(EOp::VarY, +1); return true; }
				if (Tok.Text == TEXT("z")) { Emit(EOp::VarZ, +1); return true; }
				if (Tok.Text == TEXT("u")) { Emit(EOp::VarU, +1); return true; }
				if (Tok.Text == TEXT("v")) { Emit(EOp::VarV, +1); return true; }
				if (Tok.Text == TEXT("pi")) { Emit(EOp::Const, +1, UE_DOUBLE_PI); return true; }
				if (Tok.Text == TEXT("tau")) { Emit(EOp::Const, +1, UE_DOUBLE_TWO_PI); return true; }
				if (Tok.Text == TEXT("e")) { Emit(EOp::Const, +1, UE_DOUBLE_EULERS_NUMBER); return true; }

				return Fail(FString::Printf(TEXT("Variable inconnue '%s' (variables : x y z u v)"), *Tok.Text));
			}

			if (Tok.Type == ETok::End)
			{
				return Fail(TEXT("Expression incomplète"));
			}
			return Fail(FString::Printf(TEXT("Symbole inattendu '%s'"), *Tok.Text));
		}

		const FString& Source;
		TArray<FInstr>& Code;
		TArray<FToken> Tokens;
		int32 Index = 0;
		int32 Depth = 0;
		int32 MaxDepth = 0;
		FString Error;
	};
}

bool FPatchTerrainExpression::Compile(const FString& Source, FString& OutError)
{
	Code.Reset();
	OutError.Reset();

	PatchTerrainExpr::FParser Parser(Source, Code);
	if (!Parser.Run(OutError))
	{
		Code.Reset();
		return false;
	}
	return true;
}

double FPatchTerrainExpression::Evaluate(const FContext& Ctx) const
{
	using PatchTerrainExpr::EFunc;

	if (Code.Num() == 0)
	{
		return 0.0;
	}

	double Stack[MaxStackDepth];
	int32 Top = 0; // nombre d'éléments sur la pile

	for (const FInstr& In : Code)
	{
		switch (In.Op)
		{
		case EOp::Const: Stack[Top++] = In.Value; break;
		case EOp::VarX: Stack[Top++] = Ctx.X; break;
		case EOp::VarY: Stack[Top++] = Ctx.Y; break;
		case EOp::VarZ: Stack[Top++] = Ctx.Z; break;
		case EOp::VarU: Stack[Top++] = Ctx.U; break;
		case EOp::VarV: Stack[Top++] = Ctx.V; break;

		case EOp::Add: --Top; Stack[Top - 1] += Stack[Top]; break;
		case EOp::Sub: --Top; Stack[Top - 1] -= Stack[Top]; break;
		case EOp::Mul: --Top; Stack[Top - 1] *= Stack[Top]; break;
		case EOp::Div: --Top; Stack[Top - 1] = Stack[Top] != 0.0 ? Stack[Top - 1] / Stack[Top] : 0.0; break;
		case EOp::Mod: --Top; Stack[Top - 1] = PatchTerrainExpr::PositiveMod(Stack[Top - 1], Stack[Top]); break;
		case EOp::Pow: --Top; Stack[Top - 1] = FMath::Pow(Stack[Top - 1], Stack[Top]); break;
		case EOp::Neg: Stack[Top - 1] = -Stack[Top - 1]; break;
		case EOp::Less: --Top; Stack[Top - 1] = Stack[Top - 1] < Stack[Top] ? 1.0 : 0.0; break;
		case EOp::Greater: --Top; Stack[Top - 1] = Stack[Top - 1] > Stack[Top] ? 1.0 : 0.0; break;

		case EOp::Call:
		{
			switch (static_cast<EFunc>(In.Func))
			{
			// 1 argument : remplace le sommet
			case EFunc::Sin: Stack[Top - 1] = FMath::Sin(Stack[Top - 1]); break;
			case EFunc::Cos: Stack[Top - 1] = FMath::Cos(Stack[Top - 1]); break;
			case EFunc::Tan: Stack[Top - 1] = FMath::Tan(Stack[Top - 1]); break;
			case EFunc::Asin: Stack[Top - 1] = FMath::Asin(FMath::Clamp(Stack[Top - 1], -1.0, 1.0)); break;
			case EFunc::Acos: Stack[Top - 1] = FMath::Acos(FMath::Clamp(Stack[Top - 1], -1.0, 1.0)); break;
			case EFunc::Atan: Stack[Top - 1] = FMath::Atan(Stack[Top - 1]); break;
			case EFunc::Abs: Stack[Top - 1] = FMath::Abs(Stack[Top - 1]); break;
			case EFunc::Sqrt: Stack[Top - 1] = FMath::Sqrt(FMath::Max(Stack[Top - 1], 0.0)); break;
			case EFunc::Exp: Stack[Top - 1] = FMath::Exp(Stack[Top - 1]); break;
			case EFunc::Log: Stack[Top - 1] = Stack[Top - 1] > 0.0 ? FMath::Loge(Stack[Top - 1]) : 0.0; break;
			case EFunc::Floor: Stack[Top - 1] = FMath::FloorToDouble(Stack[Top - 1]); break;
			case EFunc::Ceil: Stack[Top - 1] = FMath::CeilToDouble(Stack[Top - 1]); break;
			case EFunc::Round: Stack[Top - 1] = FMath::RoundToDouble(Stack[Top - 1]); break;
			case EFunc::Frac: Stack[Top - 1] = FMath::Frac(Stack[Top - 1]); break;
			case EFunc::Sign: Stack[Top - 1] = FMath::Sign(Stack[Top - 1]); break;

			// 2 arguments
			case EFunc::Atan2: --Top; Stack[Top - 1] = FMath::Atan2(Stack[Top - 1], Stack[Top]); break;
			case EFunc::Pow: --Top; Stack[Top - 1] = FMath::Pow(Stack[Top - 1], Stack[Top]); break;
			case EFunc::Min: --Top; Stack[Top - 1] = FMath::Min(Stack[Top - 1], Stack[Top]); break;
			case EFunc::Max: --Top; Stack[Top - 1] = FMath::Max(Stack[Top - 1], Stack[Top]); break;
			case EFunc::Mod: --Top; Stack[Top - 1] = PatchTerrainExpr::PositiveMod(Stack[Top - 1], Stack[Top]); break;
			case EFunc::Step: --Top; Stack[Top - 1] = Stack[Top] >= Stack[Top - 1] ? 1.0 : 0.0; break;
			case EFunc::Value:
				--Top;
				Stack[Top - 1] = PatchTerrainNoise::Value(FVector2D(Stack[Top - 1], Stack[Top]) + Ctx.NoiseOffset);
				break;
			case EFunc::Noise:
				--Top;
				Stack[Top - 1] = PatchTerrainNoise::Perlin(FVector2D(Stack[Top - 1], Stack[Top]) + Ctx.NoiseOffset);
				break;

			// 3 arguments
			case EFunc::Clamp:
				Top -= 2;
				Stack[Top - 1] = FMath::Clamp(Stack[Top - 1], FMath::Min(Stack[Top], Stack[Top + 1]), FMath::Max(Stack[Top], Stack[Top + 1]));
				break;
			case EFunc::Lerp:
				Top -= 2;
				Stack[Top - 1] = FMath::Lerp(Stack[Top - 1], Stack[Top], Stack[Top + 1]);
				break;
			case EFunc::SmoothStep:
			{
				Top -= 2;
				const double A = Stack[Top - 1];
				const double B = Stack[Top];
				const double X = Stack[Top + 1];
				const double T = B != A ? FMath::Clamp((X - A) / (B - A), 0.0, 1.0) : (X >= A ? 1.0 : 0.0);
				Stack[Top - 1] = T * T * (3.0 - 2.0 * T);
				break;
			}
			case EFunc::Fbm:
			case EFunc::Ridged:
			case EFunc::ValueFbm:
			{
				Top -= 2;
				const FVector2D Q = FVector2D(Stack[Top - 1], Stack[Top]) + Ctx.NoiseOffset;
				const int32 Octaves = FMath::Clamp(FMath::RoundToInt32(Stack[Top + 1]), 1, 10);
				const EFunc Func = static_cast<EFunc>(In.Func);
				Stack[Top - 1] = Func == EFunc::Fbm ? PatchTerrainNoise::FBm(Q, Octaves, 0.5)
					: Func == EFunc::Ridged ? PatchTerrainNoise::Ridged(Q, Octaves, 0.5)
					: PatchTerrainNoise::ValueFBm(Q, Octaves, 0.5);
				break;
			}
			}
			break;
		}
		}
	}

	const double Result = Top > 0 ? Stack[Top - 1] : 0.0;
	return FMath::IsFinite(Result) ? Result : 0.0;
}
