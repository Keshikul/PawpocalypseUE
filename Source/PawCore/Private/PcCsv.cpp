#include "PcCsv.h"

#include <charconv>
#include <cmath>
#include <fstream>
#include <sstream>

namespace Pc
{
	int CsvTable::Column(const std::string& Name) const
	{
		for (size_t I = 0; I < Header.size(); ++I)
		{
			if (Header[I] == Name)
			{
				return static_cast<int>(I);
			}
		}
		return -1;
	}

	bool ParseCsv(const std::string& Text, CsvTable& Out, std::string& Error)
	{
		Out = CsvTable();
		std::vector<std::vector<std::string>> Records;
		std::vector<std::string> Record;
		std::string Cell;
		bool bQuoted = false;
		bool bCellStarted = false;
		size_t I = 0;
		// A UTF-8 BOM would otherwise become part of the first header name.
		if (Text.size() >= 3 && static_cast<unsigned char>(Text[0]) == 0xEF && static_cast<unsigned char>(Text[1]) == 0xBB
			&& static_cast<unsigned char>(Text[2]) == 0xBF)
		{
			I = 3;
		}
		auto EndRecord = [&]()
		{
			Record.push_back(Cell);
			Cell.clear();
			bCellStarted = false;
			// A blank line (one empty cell) is not a record.
			if (!(Record.size() == 1 && Record[0].empty()))
			{
				Records.push_back(Record);
			}
			Record.clear();
		};
		for (; I < Text.size(); ++I)
		{
			const char C = Text[I];
			if (bQuoted)
			{
				if (C == '"')
				{
					if (I + 1 < Text.size() && Text[I + 1] == '"')
					{
						Cell += '"';
						++I;
					}
					else
					{
						bQuoted = false;
					}
				}
				else
				{
					Cell += C;
				}
				continue;
			}
			if (C == '"' && !bCellStarted)
			{
				bQuoted = true;
				bCellStarted = true;
			}
			else if (C == ',')
			{
				Record.push_back(Cell);
				Cell.clear();
				bCellStarted = false;
			}
			else if (C == '\r')
			{
				continue;
			}
			else if (C == '\n')
			{
				EndRecord();
			}
			else
			{
				Cell += C;
				bCellStarted = true;
			}
		}
		if (bQuoted)
		{
			Error = "unterminated quoted cell";
			return false;
		}
		if (bCellStarted || !Record.empty())
		{
			EndRecord();
		}
		if (Records.empty())
		{
			Error = "no header";
			return false;
		}
		Out.Header = Records[0];
		for (size_t R = 1; R < Records.size(); ++R)
		{
			if (Records[R].size() != Out.Header.size())
			{
				Error = "row " + std::to_string(R) + " has " + std::to_string(Records[R].size()) + " cells, header has "
					+ std::to_string(Out.Header.size());
				return false;
			}
			Out.Rows.push_back(Records[R]);
		}
		return true;
	}

	bool ReadTextFile(const std::string& Path, std::string& Out)
	{
		std::ifstream File(Path, std::ios::binary);
		if (!File)
		{
			return false;
		}
		std::ostringstream S;
		S << File.rdbuf();
		Out = S.str();
		return true;
	}

	bool ParseDouble(const std::string& Text, double& Out)
	{
		double V = 0.0;
		const char* Begin = Text.data();
		const char* End = Begin + Text.size();
		// from_chars rejects a leading '+', which no exporter writes; accepting it would hide a format change.
		const std::from_chars_result R = std::from_chars(Begin, End, V);
		if (R.ec != std::errc() || R.ptr != End || !std::isfinite(V))
		{
			return false;
		}
		Out = V;
		return true;
	}

	bool ParseInt(const std::string& Text, int64_t& Out)
	{
		int64_t V = 0;
		const char* Begin = Text.data();
		const char* End = Begin + Text.size();
		const std::from_chars_result R = std::from_chars(Begin, End, V);
		if (R.ec != std::errc() || R.ptr != End)
		{
			return false;
		}
		Out = V;
		return true;
	}

	namespace
	{
		std::vector<std::string> SplitList(const std::string& Text)
		{
			std::vector<std::string> Parts;
			std::string Cur;
			for (char C : Text)
			{
				if (C == ';')
				{
					Parts.push_back(Cur);
					Cur.clear();
				}
				else
				{
					Cur += C;
				}
			}
			Parts.push_back(Cur);
			return Parts;
		}
	}

	CsvRowReader::CsvRowReader(const CsvTable& InTable, size_t InRow, const char* InClass, std::vector<std::string>& InErrors)
		: Table(InTable), Row(InRow), Class(InClass), Errors(InErrors)
	{
	}

	const std::string* CsvRowReader::Cell(const char* Col) const
	{
		const int C = Table.Column(Col);
		if (C < 0 || Row >= Table.Rows.size())
		{
			return nullptr;
		}
		const std::string& Text = Table.Rows[Row][static_cast<size_t>(C)];
		return Text.empty() ? nullptr : &Text;
	}

	void CsvRowReader::Fail(const char* Col, const std::string& Why) const
	{
		const int IdCol = Table.Column("id");
		const std::string Id = IdCol >= 0 ? Table.Rows[Row][static_cast<size_t>(IdCol)] : std::string("?");
		Errors.push_back(std::string(Class) + " row " + std::to_string(Row + 1) + " (" + Id + ") " + Col + ": " + Why);
	}

	void CsvRowReader::ReadString(const char* Col, std::string& Out) const
	{
		if (const std::string* T = Cell(Col))
		{
			Out = *T;
		}
	}

	void CsvRowReader::ReadDouble(const char* Col, double& Out) const
	{
		if (const std::string* T = Cell(Col))
		{
			if (!ParseDouble(*T, Out))
			{
				Fail(Col, "not a finite number '" + *T + "'");
			}
		}
	}

	void CsvRowReader::ReadInt(const char* Col, int64_t& Out) const
	{
		if (const std::string* T = Cell(Col))
		{
			if (!ParseInt(*T, Out))
			{
				Fail(Col, "not an integer '" + *T + "'");
			}
		}
	}

	void CsvRowReader::ReadBool(const char* Col, bool& Out) const
	{
		if (const std::string* T = Cell(Col))
		{
			if (*T == "true") { Out = true; }
			else if (*T == "false") { Out = false; }
			else { Fail(Col, "not true/false '" + *T + "'"); }
		}
	}

	void CsvRowReader::ReadColor(const char* Col, std::array<double, 4>& Out) const
	{
		if (const std::string* T = Cell(Col))
		{
			const std::vector<std::string> Parts = SplitList(*T);
			std::array<double, 4> V = { 1.0, 1.0, 1.0, 1.0 };
			if (Parts.size() < 3 || Parts.size() > 4)
			{
				Fail(Col, "colour needs 3 or 4 components '" + *T + "'");
				return;
			}
			for (size_t I = 0; I < Parts.size(); ++I)
			{
				if (!ParseDouble(Parts[I], V[I]))
				{
					Fail(Col, "bad colour component '" + Parts[I] + "'");
					return;
				}
			}
			Out = V;
		}
	}

	void CsvRowReader::ReadDoubleList(const char* Col, std::vector<double>& Out) const
	{
		if (const std::string* T = Cell(Col))
		{
			std::vector<double> V;
			for (const std::string& P : SplitList(*T))
			{
				double D = 0.0;
				if (!ParseDouble(P, D))
				{
					Fail(Col, "bad list number '" + P + "'");
					return;
				}
				V.push_back(D);
			}
			Out = V;
		}
	}

	void CsvRowReader::ReadStringList(const char* Col, std::vector<std::string>& Out) const
	{
		if (const std::string* T = Cell(Col))
		{
			Out = SplitList(*T);
		}
	}

	void CsvRowReader::ReadLadderMap(const char* Col, std::map<std::string, std::vector<double>>& Out) const
	{
		const std::string* T = Cell(Col);
		if (!T)
		{
			return;
		}
		// Minimal JSON for exactly this shape: { "name": [n, n, ...], ... }. Anything else is an error, not a guess.
		const std::string& S = *T;
		size_t P = 0;
		auto Skip = [&]() { while (P < S.size() && (S[P] == ' ' || S[P] == '\t' || S[P] == '\n' || S[P] == '\r')) { ++P; } };
		auto Expect = [&](char C) { Skip(); if (P < S.size() && S[P] == C) { ++P; return true; } return false; };
		std::map<std::string, std::vector<double>> Result;
		if (!Expect('{'))
		{
			Fail(Col, "ladder map must be a JSON object");
			return;
		}
		Skip();
		if (Expect('}'))
		{
			Out = Result;
			return;
		}
		while (true)
		{
			if (!Expect('"'))
			{
				Fail(Col, "expected a quoted key");
				return;
			}
			const size_t KeyEnd = S.find('"', P);
			if (KeyEnd == std::string::npos)
			{
				Fail(Col, "unterminated key");
				return;
			}
			const std::string Key = S.substr(P, KeyEnd - P);
			P = KeyEnd + 1;
			if (!Expect(':') || !Expect('['))
			{
				Fail(Col, "expected : [ after key " + Key);
				return;
			}
			std::vector<double> Values;
			Skip();
			if (!Expect(']'))
			{
				while (true)
				{
					Skip();
					size_t NumEnd = P;
					while (NumEnd < S.size() && S[NumEnd] != ',' && S[NumEnd] != ']' && S[NumEnd] != ' ')
					{
						++NumEnd;
					}
					double D = 0.0;
					if (!ParseDouble(S.substr(P, NumEnd - P), D))
					{
						Fail(Col, "bad number in ladder " + Key);
						return;
					}
					Values.push_back(D);
					P = NumEnd;
					if (Expect(']'))
					{
						break;
					}
					if (!Expect(','))
					{
						Fail(Col, "expected , or ] in ladder " + Key);
						return;
					}
				}
			}
			Result[Key] = Values;
			if (Expect('}'))
			{
				break;
			}
			if (!Expect(','))
			{
				Fail(Col, "expected , or } after ladder " + Key);
				return;
			}
		}
		Out = Result;
	}
}
