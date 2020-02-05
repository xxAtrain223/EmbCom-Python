#ifndef SERIALBUFFER_HPP
#define SERIALBUFFER_HPP

#include <cstdio>

#include <EmbMessenger/IBuffer.hpp>
#include <EmbMessenger/DataType.hpp>
#include <serial/serial.h>

template <uint8_t BufferSize>
class SerialBuffer : public emb::shared::IBuffer
{
protected:
    serial::Serial m_serial;
    uint8_t m_buffer[BufferSize];
    uint8_t m_streamFront, m_readFront;
    uint8_t m_numberMessages;

    emb::shared::DataType m_readType, m_writeType;
    uint8_t m_readBytes, m_writeBytes;

public:
    SerialBuffer(std::string port) :
        m_serial(port)
    {
        zero();
    }

    void writeByte(const uint8_t byte) override
    {
        m_serial.write(&byte, 1);
    }

    uint8_t peek() const override
    {
        return m_buffer[m_readFront];
    }

    uint8_t readByte() override
    {
        uint8_t byte = m_buffer[m_readFront];

        if (m_readBytes == 0)
        {
            m_readType = (emb::shared::DataType)byte;
            m_readBytes = emb::shared::dataBytes(m_readType);
        }
        else
        {
            --m_readBytes;
        }

        if (m_readBytes == 0 && m_readType == emb::shared::DataType::kEndOfMessage)
        {
            --m_numberMessages;
        }

        m_readFront = (m_readFront + 1) % BufferSize;
        return byte;
    }

    bool empty() const override
    {
        return m_streamFront == m_readFront;
    }

    size_t size() const override
    {
        return (BufferSize + m_streamFront - m_readFront) % BufferSize;
    }

    uint8_t messages() const override
    {
        return m_numberMessages;
    }

    void update() override
    {
        while (m_serial.available() > 0)
        {
            m_serial.read(m_buffer + m_streamFront, 1);
            uint8_t byte = m_buffer[m_streamFront];

            if (m_writeBytes == 0)
            {
                m_writeType = (emb::shared::DataType)byte;
                m_writeBytes = emb::shared::dataBytes(m_writeType);
            }
            else
            {
                --m_writeBytes;
            }

            if (m_writeBytes == 0 && m_writeType == emb::shared::DataType::kEndOfMessage)
            {
                ++m_numberMessages;
            }

            m_streamFront = (m_streamFront + 1) % BufferSize;
        }
    }

    void zero() override
    {
        m_streamFront = 0;
        m_readFront = 0;
        m_numberMessages = 0;

        m_readType = emb::shared::DataType::kNull;
        m_readBytes = 0;
        m_writeType = emb::shared::DataType::kNull;
        m_writeBytes = 0;

        for (uint8_t i = 0; i < BufferSize; i++)
        {
            m_buffer[i] = 0;
        }
    }

    void print() const override
    {
        for (int i = 0; i < BufferSize; i += 16)
        {
            for (int j = 0; j < 16; j++)
            {
                printf("%02X ", m_buffer[i + j]);
            }
            printf("\n");
        }
        fflush(stdout);
    }
};

#endif // SERIALBUFFER_HPP