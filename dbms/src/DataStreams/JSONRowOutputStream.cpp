// Copyright 2023 PingCAP, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <DataStreams/JSONRowOutputStream.h>
#include <IO/WriteBufferValidUTF8.h>
#include <IO/WriteHelpers.h>


namespace DB
{

JSONRowOutputStream::JSONRowOutputStream(
    WriteBuffer & ostr_,
    const Block & sample_,
    bool write_statistics_,
    const FormatSettingsJSON & settings_)
    : dst_ostr(ostr_)
    , write_statistics(write_statistics_)
    , settings(settings_)
{
    NamesAndTypesList columns(sample_.getNamesAndTypesList());
    fields.assign(columns.begin(), columns.end());

    bool need_validate_utf8 = false;
    for (size_t i = 0; i < sample_.columns(); ++i)
    {
        if (!sample_.getByPosition(i).type->textCanContainOnlyValidUTF8())
            need_validate_utf8 = true;

        WriteBufferFromOwnString out;
        writeJSONString(fields[i].name, out);

        fields[i].name = out.str();
    }

    if (need_validate_utf8)
    {
        validating_ostr = std::make_unique<WriteBufferValidUTF8>(dst_ostr);
        ostr = validating_ostr.get();
    }
    else
        ostr = &dst_ostr;
}


void JSONRowOutputStream::writePrefix()
{
    writeCString("{\n", *ostr);
    writeCString("\t\"meta\":\n", *ostr);
    writeCString("\t[\n", *ostr);

    for (size_t i = 0; i < fields.size(); ++i)
    {
        writeCString("\t\t{\n", *ostr);

        writeCString("\t\t\t\"name\": ", *ostr);
        writeString(fields[i].name, *ostr);
        writeCString(",\n", *ostr);
        writeCString("\t\t\t\"type\": ", *ostr);
        writeJSONString(fields[i].type->getName(), *ostr);
        writeChar('\n', *ostr);

        writeCString("\t\t}", *ostr);
        if (i + 1 < fields.size())
            writeChar(',', *ostr);
        writeChar('\n', *ostr);
    }

    writeCString("\t],\n", *ostr);
    writeChar('\n', *ostr);
    writeCString("\t\"data\":\n", *ostr);
    writeCString("\t[\n", *ostr);
}


void JSONRowOutputStream::writeField(const IColumn & column, const IDataType & type, size_t row_num)
{
    writeCString("\t\t\t", *ostr);
    writeString(fields[field_number].name, *ostr);
    writeCString(": ", *ostr);
    type.serializeTextJSON(column, row_num, *ostr, settings);
    ++field_number;
}


void JSONRowOutputStream::writeFieldDelimiter()
{
    writeCString(",\n", *ostr);
}


void JSONRowOutputStream::writeRowStartDelimiter()
{
    if (row_count > 0)
        writeCString(",\n", *ostr);
    writeCString("\t\t{\n", *ostr);
}


void JSONRowOutputStream::writeRowEndDelimiter()
{
    writeChar('\n', *ostr);
    writeCString("\t\t}", *ostr);
    field_number = 0;
    ++row_count;
}


void JSONRowOutputStream::writeSuffix()
{
    writeChar('\n', *ostr);
    writeCString("\t]", *ostr);

    writeExtremes();

    writeCString(",\n\n", *ostr);
    writeCString("\t\"rows\": ", *ostr);
    writeIntText(row_count, *ostr);

    writeRowsBeforeLimitAtLeast();

    if (write_statistics)
        writeStatistics();

    writeChar('\n', *ostr);
    writeCString("}\n", *ostr);
    ostr->next();
}

void JSONRowOutputStream::writeRowsBeforeLimitAtLeast()
{
    if (applied_limit)
    {
        writeCString(",\n\n", *ostr);
        writeCString("\t\"rows_before_limit_at_least\": ", *ostr);
        writeIntText(rows_before_limit, *ostr);
    }
}

static void writeExtremesElement(
    const char * title,
    const Block & extremes,
    size_t row_num,
    WriteBuffer & ostr,
    const FormatSettingsJSON & settings)
{
    writeCString("\t\t\"", ostr);
    writeCString(title, ostr);
    writeCString("\":\n", ostr);
    writeCString("\t\t{\n", ostr);

    size_t extremes_columns = extremes.columns();
    for (size_t i = 0; i < extremes_columns; ++i)
    {
        const ColumnWithTypeAndName & column = extremes.safeGetByPosition(i);

        if (i != 0)
            writeCString(",\n", ostr);

        writeCString("\t\t\t", ostr);
        writeJSONString(column.name, ostr);
        writeCString(": ", ostr);
        column.type->serializeTextJSON(*column.column.get(), row_num, ostr, settings);
    }

    writeChar('\n', ostr);
    writeCString("\t\t}", ostr);
}

void JSONRowOutputStream::writeExtremes()
{
    if (extremes)
    {
        writeCString(",\n", *ostr);
        writeChar('\n', *ostr);
        writeCString("\t\"extremes\":\n", *ostr);
        writeCString("\t{\n", *ostr);

        writeExtremesElement("min", extremes, 0, *ostr, settings);
        writeCString(",\n", *ostr);
        writeExtremesElement("max", extremes, 1, *ostr, settings);

        writeChar('\n', *ostr);
        writeCString("\t}", *ostr);
    }
}


void JSONRowOutputStream::onProgress(const Progress & value)
{
    progress.incrementPiecewiseAtomically(value);
}


void JSONRowOutputStream::writeStatistics()
{
    writeCString(",\n\n", *ostr);
    writeCString("\t\"statistics\":\n", *ostr);
    writeCString("\t{\n", *ostr);

    writeCString("\t\t\"elapsed\": ", *ostr);
    writeText(watch.elapsedSeconds(), *ostr);
    writeCString(",\n", *ostr);
    writeCString("\t\t\"rows_read\": ", *ostr);
    writeText(progress.rows.load(), *ostr);
    writeCString(",\n", *ostr);
    writeCString("\t\t\"bytes_read\": ", *ostr);
    writeText(progress.bytes.load(), *ostr);
    writeChar('\n', *ostr);

    writeCString("\t}", *ostr);
}

} // namespace DB
