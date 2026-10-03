# =====================================================================================
# ConvertTonyMcMapfaceLut.ps1 — convertit une LUT 3D .dds en DDS volume RGBA16F importable par Unreal
#
# Pourquoi : la LUT de Tony McMapface (https://github.com/h3r2tic/tony-mc-mapface, shader/tony_mc_mapface.dds)
# est stockée dans un format compact (RGB9E5 ou autre) qu'Unreal 5.8 n'importe pas
# (DDSFile.cpp : seuls RGBA32F, RGBA16F, R32F, R16F, 8 et 16 bits entiers sont acceptés).
# Unreal importe en revanche directement un DDS VOLUME en RGBA16F : il devient une Volume Texture.
#
# Formats d'entrée gérés : R9G9B9E5_SHAREDEXP, R11G11B10_FLOAT, R16G16B16A16_FLOAT,
# R32G32B32A32_FLOAT, R32G32B32_FLOAT (en-tête DX10), et A16B16G16R16F / A32B32G32R32F (ancien en-tête).
#
# Utilisation (PowerShell) :
#   .\ConvertTonyMcMapfaceLut.ps1 -InputPath "C:\...\tony_mc_mapface.dds" -OutputPath "C:\...\T_TonyMcMapface.dds"
# =====================================================================================

param(
	[Parameter(Mandatory = $true)] [string] $InputPath,
	[Parameter(Mandatory = $true)] [string] $OutputPath
)

$ErrorActionPreference = 'Stop'

Add-Type -Language CSharp -TypeDefinition @'
using System;
using System.IO;

public static class DwamLutConverter
{
	const uint DDSD_CAPS = 0x1, DDSD_HEIGHT = 0x2, DDSD_WIDTH = 0x4, DDSD_PITCH = 0x8, DDSD_PIXELFORMAT = 0x1000, DDSD_DEPTH = 0x800000;
	const uint DDPF_FOURCC = 0x4;
	const uint DDSCAPS_COMPLEX = 0x8, DDSCAPS_TEXTURE = 0x1000;
	const uint DDSCAPS2_VOLUME = 0x200000;
	const uint FOURCC_DX10 = 0x30315844; // "DX10"

	// --- Conversions de formats -------------------------------------------------------

	static float DecodeRGB9E5Channel(uint Mantissa, int Exponent)
	{
		return (float)(Mantissa * Math.Pow(2.0, Exponent - 15 - 9));
	}

	static float HalfToFloat(ushort H)
	{
		int Sign = (H >> 15) & 1;
		int Exp = (H >> 10) & 0x1F;
		int Mant = H & 0x3FF;
		double V;
		if (Exp == 0) V = Mant * Math.Pow(2.0, -24);
		else if (Exp == 31) V = (Mant == 0) ? double.PositiveInfinity : double.NaN;
		else V = (1.0 + Mant / 1024.0) * Math.Pow(2.0, Exp - 15);
		return (float)(Sign == 1 ? -V : V);
	}

	static float Float11Or10ToFloat(uint Bits, int MantBits)
	{
		uint Exp = Bits >> MantBits;
		uint Mant = Bits & ((1u << MantBits) - 1);
		if (Exp == 0) return (float)(Mant * Math.Pow(2.0, -14 - MantBits));
		if (Exp == 31) return float.PositiveInfinity;
		return (float)((1.0 + Mant / (double)(1 << MantBits)) * Math.Pow(2.0, (int)Exp - 15));
	}

	public static ushort FloatToHalf(float F)
	{
		if (float.IsNaN(F)) return 0x7E00;
		uint Bits = BitConverter.ToUInt32(BitConverter.GetBytes(F), 0);
		uint Sign = (Bits >> 16) & 0x8000;
		int Exp = (int)((Bits >> 23) & 0xFF) - 127 + 15;
		uint Mant = Bits & 0x7FFFFF;
		if (Exp >= 31) return (ushort)(Sign | 0x7C00);               // trop grand : infini
		if (Exp <= 0)
		{
			if (Exp < -10) return (ushort)Sign;                          // trop petit : zéro
			Mant |= 0x800000;                                            // sous-normal
			int Shift = 14 - Exp;
			uint HalfMant = Mant >> Shift;
			if (((Mant >> (Shift - 1)) & 1) != 0) HalfMant++;            // arrondi
			return (ushort)(Sign | HalfMant);
		}
		uint Result = Sign | ((uint)Exp << 10) | (Mant >> 13);
		if ((Mant & 0x1000) != 0) Result++;                              // arrondi (peut monter l'exposant, c'est correct)
		return (ushort)Result;
	}

	// --- Conversion -------------------------------------------------------------------

	public static string Convert(string InputPath, string OutputPath)
	{
		byte[] Data = File.ReadAllBytes(InputPath);
		if (Data.Length < 128 || BitConverter.ToUInt32(Data, 0) != 0x20534444) // "DDS "
			throw new Exception("Ce n'est pas un fichier DDS.");

		uint Height = BitConverter.ToUInt32(Data, 12);
		uint Width  = BitConverter.ToUInt32(Data, 16);
		uint Depth  = BitConverter.ToUInt32(Data, 24);
		uint PfFlags = BitConverter.ToUInt32(Data, 80);
		uint FourCC  = BitConverter.ToUInt32(Data, 84);
		uint Caps2   = BitConverter.ToUInt32(Data, 112);
		if ((Caps2 & DDSCAPS2_VOLUME) == 0 || Depth == 0) Depth = 1;

		int Offset = 128;
		uint Format = 0;           // DXGI_FORMAT
		if ((PfFlags & DDPF_FOURCC) != 0 && FourCC == FOURCC_DX10)
		{
			Format = BitConverter.ToUInt32(Data, 128);
			uint Dimension = BitConverter.ToUInt32(Data, 132);
			if (Dimension == 4 && Depth == 1) Depth = BitConverter.ToUInt32(Data, 24);
			Offset = 148;
		}
		else if ((PfFlags & DDPF_FOURCC) != 0 && FourCC == 113) Format = 10; // A16B16G16R16F
		else if ((PfFlags & DDPF_FOURCC) != 0 && FourCC == 116) Format = 2;  // A32B32G32R32F
		else throw new Exception("Format DDS non géré (FourCC " + FourCC + ").");

		int Bpp;
		switch (Format)
		{
			case 67: case 26: Bpp = 4; break;   // R9G9B9E5, R11G11B10
			case 10: Bpp = 8; break;            // RGBA16F
			case 2:  Bpp = 16; break;           // RGBA32F
			case 6:  Bpp = 12; break;           // RGB32F
			default: throw new Exception("DXGI_FORMAT " + Format + " non géré.");
		}

		long Count = (long)Width * Height * Depth;
		if (Offset + Count * Bpp > Data.Length) throw new Exception("Fichier tronqué.");

		float MaxValue = 0.0f;
		byte[] Out = new byte[148 + Count * 8];
		for (long I = 0; I < Count; ++I)
		{
			int P = (int)(Offset + I * Bpp);
			float R, G, B, A = 1.0f;
			if (Format == 67)
			{
				uint V = BitConverter.ToUInt32(Data, P);
				int E = (int)(V >> 27);
				R = DecodeRGB9E5Channel(V & 0x1FF, E);
				G = DecodeRGB9E5Channel((V >> 9) & 0x1FF, E);
				B = DecodeRGB9E5Channel((V >> 18) & 0x1FF, E);
			}
			else if (Format == 26)
			{
				uint V = BitConverter.ToUInt32(Data, P);
				R = Float11Or10ToFloat(V & 0x7FF, 6);
				G = Float11Or10ToFloat((V >> 11) & 0x7FF, 6);
				B = Float11Or10ToFloat((V >> 22) & 0x3FF, 5);
			}
			else if (Format == 10)
			{
				R = HalfToFloat(BitConverter.ToUInt16(Data, P));
				G = HalfToFloat(BitConverter.ToUInt16(Data, P + 2));
				B = HalfToFloat(BitConverter.ToUInt16(Data, P + 4));
				A = HalfToFloat(BitConverter.ToUInt16(Data, P + 6));
			}
			else
			{
				R = BitConverter.ToSingle(Data, P);
				G = BitConverter.ToSingle(Data, P + 4);
				B = BitConverter.ToSingle(Data, P + 8);
				if (Format == 2) A = BitConverter.ToSingle(Data, P + 12);
			}
			MaxValue = Math.Max(MaxValue, Math.Max(R, Math.Max(G, B)));

			int Q = (int)(148 + I * 8);
			WriteU16(Out, Q,     FloatToHalf(R));
			WriteU16(Out, Q + 2, FloatToHalf(G));
			WriteU16(Out, Q + 4, FloatToHalf(B));
			WriteU16(Out, Q + 6, FloatToHalf(A));
		}

		// En-tête DDS (volume, RGBA16F, en-tête DX10, un seul mip)
		WriteU32(Out, 0, 0x20534444);
		WriteU32(Out, 4, 124);
		WriteU32(Out, 8, DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_DEPTH);
		WriteU32(Out, 12, Height);
		WriteU32(Out, 16, Width);
		WriteU32(Out, 20, Width * 8);         // pitch
		WriteU32(Out, 24, Depth);
		WriteU32(Out, 28, 1);                 // mips
		WriteU32(Out, 76, 32);                // taille du pixel format
		WriteU32(Out, 80, DDPF_FOURCC);
		WriteU32(Out, 84, FOURCC_DX10);
		WriteU32(Out, 108, DDSCAPS_TEXTURE | DDSCAPS_COMPLEX);
		WriteU32(Out, 112, DDSCAPS2_VOLUME);
		WriteU32(Out, 128, 10);               // DXGI_FORMAT_R16G16B16A16_FLOAT
		WriteU32(Out, 132, 4);                // D3D10_RESOURCE_DIMENSION_TEXTURE3D
		WriteU32(Out, 136, 0);
		WriteU32(Out, 140, 1);                // arraySize
		WriteU32(Out, 144, 0);

		File.WriteAllBytes(OutputPath, Out);
		return string.Format("{0}x{1}x{2}, DXGI_FORMAT {3} -> R16G16B16A16_FLOAT, valeur max {4:0.####}", Width, Height, Depth, Format, MaxValue);
	}

	static void WriteU32(byte[] B, int P, uint V) { Array.Copy(BitConverter.GetBytes(V), 0, B, P, 4); }
	static void WriteU16(byte[] B, int P, ushort V) { Array.Copy(BitConverter.GetBytes(V), 0, B, P, 2); }
}
'@

$In  = (Resolve-Path $InputPath).Path
$Out = [System.IO.Path]::GetFullPath($OutputPath)
$Summary = [DwamLutConverter]::Convert($In, $Out)
Write-Host "LUT convertie : $Summary"
Write-Host "Fichier : $Out"
Write-Host "Dans Unreal : glisser ce .dds dans le Content Browser (il devient une Volume Texture), puis Tiling Method = Clamp."
