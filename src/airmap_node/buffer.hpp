#pragma once

// Buffer
class Buffer
{
public:
	void clear() {
		m_buffer.clear();
	}

	template <typename T>
	Buffer& add(const T& value) {
		return add(reinterpret_cast<const char*>(&value), sizeof(value));
	}

	Buffer& add(const std::string& s) {
		return add(s.c_str(), s.size());
	}

	Buffer& add(const std::vector<std::uint8_t>& v) {
		return add(reinterpret_cast<const char*>(v.data()), v.size());
	}

	Buffer& add(const char* value, std::size_t size) {
		m_buffer.insert(m_buffer.end(), value, value + size);
		return *this;
	}

	const std::string& get() const {
		return m_buffer;
	}

private:
	std::string m_buffer;
};
