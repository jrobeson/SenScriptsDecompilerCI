#include "headers/instruction.h"
#include "headers/functions.h"
#include "headers/global_vars.h"
#include "headers/utilities.h"
#include <string>

Instruction::Instruction(int addr, uint OP, Builder* Maker) {
    OPCode = OP;
    this->Maker = Maker;
    this->addr_instr = addr;
}
Instruction::Instruction(int addr, std::string name, uint OP, Builder* Maker) {
    this->addr_instr = addr;
    OPCode = OP;
    this->name = std::move(name);
    this->Maker = Maker;
}
Instruction::~Instruction() = default;
Instruction::Instruction(int& addr, int idx_row, QXlsx::Document& excelScenarioSheet, std::string name, uint OP, Builder* Maker) {
    this->name = std::move(name);
    this->OPCode = OP;
    this->Maker = Maker;
    addr_instr = addr;
    if (OPCode <= 0xFF) addr++;

    int idx_operande = 3;
    std::string type = excelScenarioSheet.read(idx_row, idx_operande).toString().toStdString();
    while (!type.empty()) {

        QByteArray Value;
        operande op;
        if (type == "int") {
            int Operande = excelScenarioSheet.read(idx_row + 1, idx_operande).toInt();

            Value = GetBytesFromInt(Operande);
            op = operande(addr, "int", Value);

        } else if (type == "float") {

            float Operande = excelScenarioSheet.read(idx_row + 1, idx_operande).toFloat();

            Value = GetBytesFromFloat(Operande);
            op = operande(addr, "float", Value);
        } else if (type == "short") {
            int16_t Operande = static_cast<int16_t>(excelScenarioSheet.read(idx_row + 1, idx_operande).toInt());
            Value = GetBytesFromShort(Operande);
            op = operande(addr, "short", Value);

        } else if ((type == "byte") || (type == "OP Code")) {
            int OP = excelScenarioSheet.read(idx_row + 1, idx_operande).toInt();

            if (OP <= 0xFF) { // actually the opposite never happens.
                auto Operande = static_cast<char>(((OP)&0x000000FF));
                Value.push_back(Operande);
                op = operande(addr, "byte", Value);
            }
        } else if (type == "fill") {

            QString Operande_prev = excelScenarioSheet.read(idx_row + 1, idx_operande - 1).toString();
            QString Operande = excelScenarioSheet.read(idx_row + 1, idx_operande).toString();
            QString str_max_length = Operande.mid(1, Operande.indexOf('-') - 1);

            // apparently it is not possible to get the result of the formula...

            for (int id = 0; id < str_max_length.toInt() - Operande_prev.toUtf8().size() - 1; id++) {
                Value.push_back('\0');
            }

            op = operande(addr, "fill", Value);

        }

        else if ((type == "string") || (type == "dialog")) {
            QString Operande = (excelScenarioSheet.read(idx_row + 1, idx_operande).toString());
            Value = Operande.toUtf8();
            QTextCodec* codec = QTextCodec::codecForName(QString::fromStdString(OutputDatFileEncoding).toUtf8());
            QByteArray Value = codec->fromUnicode(Operande);
            if (type == "string") Value.push_back('\0');
            Value.replace('\n', 1);

            op = operande(addr, type, Value);
        } else if (type == "bytearray") {

            while (type == "bytearray") {
                auto Operande = static_cast<char>(((excelScenarioSheet.read(idx_row + 1, idx_operande).toInt()) & 0x000000FF));
                Value.push_back(Operande);
                idx_operande++;
                type = excelScenarioSheet.read(idx_row, idx_operande).toString().toStdString();
            }
            op = operande(addr, "bytearray", Value);
            idx_operande--;
        } else if (type == "pointer") {
            QString Operande = (excelScenarioSheet.read(idx_row + 1, idx_operande).toString());

            QString RowPointedStr = Operande.right(Operande.length() - 2);

            int actual_row = RowPointedStr.toInt();

            Value = GetBytesFromInt(actual_row);

            op = operande(addr, "pointer", Value);
        }
        this->AddOperande(op);
        idx_operande++;
        addr = addr + op.getLength();
        type = excelScenarioSheet.read(idx_row, idx_operande).toString().toStdString();
    }
}
int Instruction::get_Nb_operandes() const { return static_cast<int>(operandes.size()); }
operande Instruction::get_operande(int i) const { return operandes[i]; }

int Instruction::get_addr_instr() const { return this->addr_instr; }
void Instruction::WriteDat() {}
int Instruction::WriteXLSX(QXlsx::Document& excelScenarioSheet, std::vector<function> funs, int row, int& col) {
    QXlsx::Format FormatInstr;
    QXlsx::Format FormatType;
    QXlsx::Format FormatOP;
    auto ColorBg = QColor(qRgb(255, 230, 210));
    FormatType.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    FormatType.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    FormatType.setTextWrap(true);
    FormatInstr.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    FormatInstr.setVerticalAlignment(QXlsx::Format::AlignVCenter);

    FormatType.setBottomBorderStyle(QXlsx::Format::BorderThin);
    FormatType.setLeftBorderStyle(QXlsx::Format::BorderThin);
    FormatType.setRightBorderStyle(QXlsx::Format::BorderThin);
    FormatType.setTopBorderStyle(QXlsx::Format::BorderThin);
    FormatType.setPatternBackgroundColor(ColorBg);

    FormatInstr.setBottomBorderStyle(QXlsx::Format::BorderThin);
    FormatInstr.setLeftBorderStyle(QXlsx::Format::BorderThin);
    FormatInstr.setRightBorderStyle(QXlsx::Format::BorderThin);
    FormatInstr.setTopBorderStyle(QXlsx::Format::BorderThin);

    FormatOP.setBottomBorderStyle(QXlsx::Format::BorderThin);
    FormatOP.setLeftBorderStyle(QXlsx::Format::BorderThin);
    FormatOP.setRightBorderStyle(QXlsx::Format::BorderThin);
    FormatOP.setTopBorderStyle(QXlsx::Format::BorderThin);

    FormatType.setFontBold(true);
    QColor color;

    color = QColor::fromHsl((int)OPCode, 255, 185, 255);
    FormatOP.setPatternBackgroundColor(color);

    excelScenarioSheet.write(row, col + 2, "OP Code", FormatType);
    excelScenarioSheet.write(row + 1, col + 2, OPCode, FormatOP);
    int col_cnt = 0;
    for (auto& operande : operandes) {

        QString type = QString::fromStdString(operande.getType());
        QByteArray Value = operande.getValue();

        if (type == "int") {
            excelScenarioSheet.write(row, col + 3 + col_cnt, type, FormatType);
            excelScenarioSheet.write(row + 1, col + 3 + col_cnt, ReadIntegerFromByteArray(0, Value), FormatInstr);
            col_cnt++;
        } else if (type == "float") {
            excelScenarioSheet.write(row, col + 3 + col_cnt, type, FormatType);
            excelScenarioSheet.write(row + 1, col + 3 + col_cnt, QByteArrayToFloat(Value), FormatInstr);
            col_cnt++;
        } else if (type == "short") {
            excelScenarioSheet.write(row, col + 3 + col_cnt, type, FormatType);
            excelScenarioSheet.write(row + 1, col + 3 + col_cnt, (uint16_t)ReadShortFromByteArray(0, Value), FormatInstr);
            col_cnt++;
        } else if (type == "byte") {

            excelScenarioSheet.write(row, col + 3 + col_cnt, type, FormatType);
            excelScenarioSheet.write(row + 1, col + 3 + col_cnt, (unsigned char)Value[0], FormatInstr);
            col_cnt++;
        } else if (type == "fill") {
            excelScenarioSheet.write(row, col + 3 + col_cnt, type, FormatType);
            excelScenarioSheet.write(row + 1,
                                     col + 3 + col_cnt,
                                     "=" + QString::number(operande.getBytesToFill()) + "-LENB(INDIRECT(ADDRESS(" +
                                       QString::number(row + 1) + "," + QString::number(3 + col_cnt - 1) + ")))",
                                     FormatInstr);
            col_cnt++;
        } else if (type == "bytearray") {

            for (auto&& idx_byte : Value) {
                excelScenarioSheet.write(row, col + 3 + col_cnt, type, FormatType);
                excelScenarioSheet.write(row + 1, col + 3 + col_cnt, (unsigned char)idx_byte, FormatInstr);
                col_cnt++;
            }

        } else if (type == "instruction") {
            int addr = 0;

            std::shared_ptr<Instruction> instr = Maker->CreateInstructionFromDAT(
              addr, Value, 0); // function type is 0 here because sub05 is only called by OP Code instructions.
            int column_instr = col + 3 + col_cnt - 2;
            int cnt = instr->WriteXLSX(excelScenarioSheet, funs, row, column_instr);

            col = col + cnt + 1;

        } else if ((type == "string") || (type == "dialog")) {

            excelScenarioSheet.write(row, col + 3 + col_cnt, type, FormatType);
            QByteArray value_ = Value;

            value_.replace(1, "\n");

            QTextCodec* codec = QTextCodec::codecForName(QString::fromStdString(InputDatFileEncoding).toUtf8());

            QString string = codec->toUnicode(value_);

            excelScenarioSheet.write(row + 1, col + 3 + col_cnt, string, FormatInstr);
            col_cnt++;
        } else if (type == "pointer") {
            excelScenarioSheet.write(row, col + 3 + col_cnt, type, FormatType);
            int ID = funs[0].ID;
            size_t nb_row = 3;
            int idx_fun = 0;
            while (ID != operande.getDestination().FunctionID) {

                nb_row = nb_row + 1; // row with function name
                nb_row = nb_row + 2 * funs[idx_fun].InstructionsInFunction.size();
                idx_fun++;
                ID = funs[idx_fun].ID;
            }
            nb_row = nb_row + 1; // row with function name
            nb_row = nb_row + 2 * (operande.getDestination().InstructionID + 1);
            QString ptrExcel = "=A" + QString::number((nb_row));

            QXlsx::Format format;
            format.setBottomBorderStyle(QXlsx::Format::BorderThin);
            format.setLeftBorderStyle(QXlsx::Format::BorderThin);
            format.setRightBorderStyle(QXlsx::Format::BorderThin);
            format.setTopBorderStyle(QXlsx::Format::BorderThin);
            format.setFontBold(true);
            auto FontColor = QColor(qRgb(255, 0, 0));
            format.setFontColor(FontColor);
            excelScenarioSheet.write(row + 1, col + 3 + col_cnt, ptrExcel, format);
            col_cnt++;
        }
    }

    return col_cnt;
}
void Instruction::set_addr_instr(int i) { addr_instr = i; }
/*This version of AddOperande is supposed to check if a string contain illegal xml characters,
but I didn't finish it yet.
If there is any illegal xml character, every string in the sheet disappears and the file can't be decompiled,
this is a problem for some broken files that we would want to restore (example is ply000 from CS3)*/
void Instruction::AddOperande(operande op) {

    QByteArray value = op.getValue();

    if (op.getType() == "string") {
        QTextCodec::ConverterState state;

        QTextCodec* codec = QTextCodec::codecForName(QString::fromStdString(InputDatFileEncoding).toUtf8());
        const QString text = codec->toUnicode(value, value.size(), &state);

        if ((state.invalidChars > 0) || text.contains('\x0B') || text.contains('\x06') || text.contains('\x07') || text.contains('\x08') ||
            text.contains('\x05') || text.contains('\x04') || text.contains('\x03') || text.contains('\x02')) {
            op.setValue(QByteArray(nullptr));

            // operandes.push_back(op);
        } else {
            operandes.push_back(op);
        }
    } else {
        operandes.push_back(op);
    }
}

int Instruction::get_length_in_bytes() { return getBytes().size(); }

uint Instruction::get_OP() const { return OPCode; }
QByteArray Instruction::getBytes() {
    QByteArray bytes;
    if (OPCode <= 0xFF) bytes.push_back((char)OPCode);
    for (auto& it : operandes) {

        QByteArray op_bytes = it.getValue();
        for (auto&& op_byte : op_bytes) {
            bytes.push_back(op_byte);
        }
    }
    return bytes;
}
