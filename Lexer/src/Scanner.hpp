#ifndef SCANNER_H_
#define SCANNER_H_
#include <array>
#include <queue>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

struct Lexeme {
    std::uint32_t label;
    std::string data;
};

template <class ArrayDFA, size_t N>
class Scanner {
public:
    using Buffer = std::array<char, N + 1>;   // 字符缓冲，最后一位为哨兵EOF
    using DualBuffer = std::array<Buffer, 2>; // 双缓冲

    // 双缓冲的迭代器
    class BufferIter : std::iterator_traits<char*> {
    public:
        BufferIter(Scanner& scanner, Buffer::iterator iter) : m_scanner(scanner), m_iter(iter) {}
        BufferIter(const BufferIter& other) : BufferIter(other.m_scanner, other.m_iter) {}

        char operator*() const { return *m_iter; }

        BufferIter& operator++() { return ++m_iter, moveBufferIfEnd(); }
        BufferIter& operator--() { return --m_iter, moveBufferIfEnd(); }
        BufferIter  operator++(int) { auto tmp = *this; ++(*this); return tmp; }
        BufferIter  operator--(int) { auto tmp = *this; --(*this); return tmp; }

        bool operator==(const BufferIter& other) const { return m_iter == other.m_iter; }
        bool operator!=(const BufferIter& other) const { return m_iter != other.m_iter; }

        BufferIter& operator=(BufferIter other) {
            if (&m_scanner == &other.m_scanner) {
                m_iter = other.m_iter;
            } else {
                throw std::invalid_argument("must in in same scanner");
            }
            return *this;
        }

    private:
        BufferIter& moveBufferIfEnd() {
            if (*m_iter == EOF) {
                m_scanner.m_bufferIndex += 1;
                m_scanner.m_bufferIndex %= m_scanner.m_buffer.size();
                m_scanner.loadBuffer();
                m_iter = m_scanner.currentBuffer().begin();
            }
            return *this;
        }

        Scanner& m_scanner;
        Buffer::iterator m_iter;
    };

    Scanner(std::initializer_list<fs::path> sources)
        : m_buffer(), 
        m_begin(*this, m_buffer[0].begin()), 
        m_forward(*this, m_buffer[0].begin()), 
        m_bufferIndex(0) 
    {
        for (auto& filePath : sources) {
            m_sources.push(filePath);
        }
        for (auto& buffer : m_buffer) {
            buffer.fill(EOF);
        }
        loadBuffer();
    }

    Buffer& currentBuffer() {
        return m_buffer[m_bufferIndex];
    }

    Lexeme nextLexeme() {
        // Initialize stack with initial state
        m_stateStack.resize(1, m_dfa.initial_state);
        // forwarding
        while (true) {
            const auto state = m_stateStack.back();
            const auto nextChar = *(++m_forward);
            if (nextChar == EOF) {
                break;
            }
            const auto nextState = m_dfa.trans(state, nextChar);
            if (nextState == m_dfa.null_state) {
                break;
            }
            m_stateStack.push_back(nextState);
        }
        // backtracking
        while (!m_stateStack.empty()) {
            const auto label = m_dfa.label(m_stateStack.back());
            m_stateStack.pop_back();
            if (label != 0) {
                const auto lexeme = std::string(m_begin, m_forward);
                m_begin = m_forward;
                return { label, lexeme };
            }
        }
        // TODO: No lexeme found error handling
        m_begin = m_forward;
        return { -1 , "" };
    }

private:
    Buffer& loadBuffer() { // 读取新的文件块至当前正使用的缓冲
        // TODO: log read status here
        while (!m_curFile.is_open()) {
            if (m_sources.empty()) {
                return currentBuffer();
            }
            const auto newFile = m_sources.front();
            m_sources.pop();
            m_curFile.open(newFile);
        }
        m_curFile.read(&currentBuffer()[0], N);
        if (m_curFile.eof()) {
            m_curFile.close();
        }
        return currentBuffer();
    }

private:
    // 文件读取相关成员
    std::queue<fs::path> m_sources;
    std::ifstream m_curFile;

    // 缓冲相关成员
    DualBuffer m_buffer;
    BufferIter m_begin;
    BufferIter m_forward;
    size_t m_bufferIndex;

    // 状态机相关成员
    ArrayDFA m_dfa;
    std::vector<int> m_stateStack;
};

#endif // !SCANNER_H_
