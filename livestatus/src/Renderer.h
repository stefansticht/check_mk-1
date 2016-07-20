// +------------------------------------------------------------------+
// |             ____ _               _        __  __ _  __           |
// |            / ___| |__   ___  ___| | __   |  \/  | |/ /           |
// |           | |   | '_ \ / _ \/ __| |/ /   | |\/| | ' /            |
// |           | |___| | | |  __/ (__|   <    | |  | | . \            |
// |            \____|_| |_|\___|\___|_|\_\___|_|  |_|_|\_\           |
// |                                                                  |
// | Copyright Mathias Kettner 2014             mk@mathias-kettner.de |
// +------------------------------------------------------------------+
//
// This file is part of Check_MK.
// The official homepage is at http://mathias-kettner.de/check_mk.
//
// check_mk is free software;  you can redistribute it and/or modify it
// under the  terms of the  GNU General Public License  as published by
// the Free Software Foundation in version 2.  check_mk is  distributed
// in the hope that it will be useful, but WITHOUT ANY WARRANTY;  with-
// out even the implied warranty of  MERCHANTABILITY  or  FITNESS FOR A
// PARTICULAR PURPOSE. See the  GNU General Public License for more de-
// tails. You should have  received  a copy of the  GNU  General Public
// License along with GNU Make; see the file  COPYING.  If  not,  write
// to the Free Software Foundation, Inc., 51 Franklin St,  Fifth Floor,
// Boston, MA 02110-1301 USA.

#ifndef Renderer_h
#define Renderer_h

#include "config.h"  // IWYU pragma: keep
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "OutputBuffer.h"
#include "global_counters.h"

enum class OutputFormat { csv, json, python };

class Renderer {
public:
    // for friend declarations
    class Row;
    class List;
    class Sublist;
    class Dict;

    class Query {
    public:
        explicit Query(Renderer &rend) : _renderer(rend), _first(true) {
            renderer().startQuery();
        }

        ~Query() { renderer().endQuery(); }

        void setError(OutputBuffer::ResponseCode code,
                      const std::string &message) {
            renderer().setError(code, message);
        }
        std::size_t size() const { return renderer().size(); }

    private:
        Renderer &_renderer;
        bool _first;

        void next() {
            if (_first) {
                _first = false;
            } else {
                renderer().separateQueryElements();
            }
        }

        Renderer &renderer() const { return _renderer; }

        // for next() and renderer()
        friend class Renderer::Row;
    };

    class Row {
    public:
        explicit Row(Query &query) : _query(query), _first(true) {
            _query.next();
            renderer().startRow();
        }

        ~Row() { renderer().endRow(); }

        void outputNull() {
            next();
            renderer().outputNull();
        }
        void outputBlob(const std::vector<char> *value) {
            next();
            renderer().outputBlob(value);
        }
        void outputString(const char *value) {
            next();
            renderer().outputString(value);
        }
        void outputCPPString(const std::string &value) {
            next();
            renderer().outputCPPString(value);
        }
        void outputInteger(int32_t value) {
            next();
            renderer().outputInteger(value);
        }
        void outputInteger64(int64_t value) {
            next();
            renderer().outputInteger64(value);
        }
        void outputTime(int32_t value) {
            next();
            renderer().outputTime(value);
        }
        void outputUnsignedLong(unsigned long value) {
            next();
            renderer().outputUnsignedLong(value);
        }
        void outputCounter(counter_t value) {
            next();
            renderer().outputCounter(value);
        }
        void outputDouble(double value) {
            next();
            renderer().outputDouble(value);
        }

    private:
        Query &_query;
        bool _first;

        void next() {
            if (_first) {
                _first = false;
            } else {
                renderer().separateRowElements();
            }
        }

        Renderer &renderer() const { return _query.renderer(); }

        // for next() and renderer()
        friend class Renderer::List;

        // for next() and renderer()
        friend class Renderer::Dict;
    };

    class List {
    public:
        explicit List(Row &row) : _row(row), _first(true) {
            _row.next();
            renderer().startList();
        }

        ~List() { renderer().endList(); }

        void outputString(const char *value) {
            next();
            renderer().outputString(value);
        }
        void outputCPPString(const std::string &value) {
            next();
            renderer().outputCPPString(value);
        }
        void outputUnsignedLong(unsigned long value) {
            next();
            renderer().outputUnsignedLong(value);
        }
        void outputTime(int32_t value) {
            next();
            renderer().outputTime(value);
        }
        void outputDouble(double value) {
            next();
            renderer().outputDouble(value);
        }

    private:
        Row &_row;
        bool _first;

        void next() {
            if (_first) {
                _first = false;
            } else {
                renderer().separateListElements();
            }
        }

        Renderer &renderer() const { return _row.renderer(); }

        // for next() and renderer()
        friend class Renderer::Sublist;
    };

    class Sublist {
    public:
        explicit Sublist(List &list) : _list(list), _first(true) {
            _list.next();
            renderer().startSublist();
        }

        ~Sublist() { renderer().endSublist(); }

        void outputCPPString(const std::string &value) {
            next();
            renderer().outputCPPString(value);
        }
        void outputInteger(int32_t value) {
            next();
            renderer().outputInteger(value);
        }
        void outputTime(int32_t value) {
            next();
            renderer().outputTime(value);
        }
        void outputUnsignedLong(unsigned long value) {
            next();
            renderer().outputUnsignedLong(value);
        }
        void outputString(const char *value) {
            next();
            renderer().outputString(value);
        }

    private:
        List &_list;
        bool _first;

        void next() {
            if (_first) {
                _first = false;
            } else {
                renderer().separateSublistElements();
            }
        }

        Renderer &renderer() const { return _list.renderer(); }
    };

    class Dict {
    public:
        explicit Dict(Renderer::Row &row) : _row(row), _first(true) {
            _row.next();
            renderer().startDict();
        }

        ~Dict() { renderer().endDict(); }

        void renderKeyValue(std::string key, std::string value) {
            next();
            renderer().outputCPPString(key);
            renderer().separateDictKeyValue();
            renderer().outputCPPString(value);
        }

    private:
        Row &_row;
        bool _first;

        void next() {
            if (_first) {
                _first = false;
            } else {
                renderer().separateDictElements();
            }
        }

        Renderer &renderer() const { return _row.renderer(); }
    };

    static std::unique_ptr<Renderer> make(
        OutputFormat format, OutputBuffer *output,
        OutputBuffer::ResponseHeader response_header, bool do_keep_alive,
        std::string invalid_header_message, std::string field_separator,
        std::string dataset_separator, std::string list_separator,
        std::string host_service_separator, int timezone_offset);

    virtual ~Renderer();

protected:
    Renderer(OutputBuffer *output, OutputBuffer::ResponseHeader response_header,
             bool do_keep_alive, std::string invalid_header_message,
             int timezone_offset);

    void add(const std::string &str);
    void add(const std::vector<char> &value);

    void outputCharsAsBlob(const char *value, std::size_t len);
    void outputCharsAsString(const char *value, std::size_t len);

private:
    OutputBuffer *const _output;
    const int _timezone_offset;

    void setError(OutputBuffer::ResponseCode code, const std::string &message);
    std::size_t size() const;

    virtual void outputNull() = 0;
    virtual void outputBlob(const std::vector<char> *value) = 0;
    virtual void outputString(const char *value) = 0;

    void outputCPPString(const std::string &value);
    void outputInteger(int32_t value);
    void outputInteger64(int64_t value);
    void outputTime(int32_t value);
    void outputUnsignedLong(unsigned long value);
    void outputCounter(counter_t value);
    void outputDouble(double value);
    void outputUnicodeEscape(unsigned value);

    // A whole query.
    virtual void startQuery() = 0;
    virtual void separateQueryElements() = 0;
    virtual void endQuery() = 0;

    // Output a single row returned by lq.
    virtual void startRow() = 0;
    virtual void separateRowElements() = 0;
    virtual void endRow() = 0;

    // Output a list-valued column.
    virtual void startList() = 0;
    virtual void separateListElements() = 0;
    virtual void endList() = 0;

    // Output a list-valued value within a list-valued column.
    virtual void startSublist() = 0;
    virtual void separateSublistElements() = 0;
    virtual void endSublist() = 0;

    // Output a dictionary, see CustomVarsColumn.
    virtual void startDict() = 0;
    virtual void separateDictElements() = 0;
    virtual void separateDictKeyValue() = 0;
    virtual void endDict() = 0;
};

#endif  // Renderer_h
