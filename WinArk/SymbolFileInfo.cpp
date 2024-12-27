#include "stdafx.h"
#include "SymbolFileInfo.h"
#include <filesystem>
#include <Helpers.h>
#include <fstream>
#include <asio.hpp>
#include <asio\ssl.hpp>


#pragma comment(lib,"Ws2_32")
#pragma comment(lib,"Iphlpapi")
#pragma comment(lib,"Crypt32")


bool SymbolFileInfo::SymDownloadSymbol(std::wstring localPath) {
	std::wstring path = localPath + L"\\" + _path.GetString();
	std::filesystem::create_directories(path);

	std::string url = "https://msdl.microsoft.com/download/symbols";

	if (url.back() != '/')
		url += '/';

	CString temp = _pdbFile + L"/" + _pdbSignature + L"/" + _pdbFile;
	std::wstring symbolUrl = temp.GetBuffer();
	url += Helpers::WstringToString(symbolUrl);
	std::wstring fileName = path + L"\\" + _pdbFile.GetString();
	bool isExist = std::filesystem::is_regular_file(fileName);
	
	if (isExist) {
		// If the symbols pdb download by SDM.exe, we just skip the check.
		std::wstring sdm = localPath + L"\\sdm.json";
		isExist = std::filesystem::is_regular_file(sdm);
		if (isExist) {
			return true;
		}

		// How to know the pdb file has downloaded completley since the last time?
		auto fileSize = std::filesystem::file_size(fileName);
		if (fileSize) {
			return true;
		}
	}

	wil::unique_handle hThread(::CreateThread(nullptr, 0, [](auto params)->DWORD {
		SymbolFileInfo* info = (SymbolFileInfo*)params;
		return 0;
		}, this, 0, nullptr));

	auto result = Download(url, fileName, "WinArk", 1000,
		[](void* userdata, unsigned long long readBytes, unsigned long long totalBytes) {

			return true;
		},
		nullptr);
	
	bool bOk = result == downslib_error::ok ? true : false;
	if (!bOk) {
		MessageBox(NULL, L"Failed init symbols,\r\nWinArk will exit...\r\n", L"WinArk", MB_ICONERROR);
		std::filesystem::remove_all(path);
	}
	return bOk;
}

bool SymbolFileInfo::GetPdbSignature(ULONG_PTR imageBase,PIMAGE_DEBUG_DIRECTORY entry) {
	if (entry->SizeOfData < sizeof(CV_INFO_PDB20))
		return false;

	ULONG_PTR offset = 0;
	
	offset = entry->PointerToRawData;
	auto cvData = (unsigned char*)(imageBase + offset);
	auto signature = *(DWORD*)cvData;

	if (signature == '01BN') {
		auto cv = (CV_INFO_PDB20*)cvData;
		_pdbSignature.Format(L"%X%X", cv->Signature, cv->Age);
		std::string file((const char*)cv->PdbFileName, entry->SizeOfData - FIELD_OFFSET(CV_INFO_PDB20, PdbFileName));
		_pdbFile = Helpers::StringToWstring(file).c_str();
		_pdbValidation.signature = cv->Signature;
		_pdbValidation.age = cv->Age;
	}
	else if (signature == 'SDSR') {
		auto cv = (CV_INFO_PDB70*)cvData;
		_pdbSignature.Format(L"%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X%X",
			cv->Signature.Data1, cv->Signature.Data2, cv->Signature.Data3,
			cv->Signature.Data4[0], cv->Signature.Data4[1], cv->Signature.Data4[2],
			cv->Signature.Data4[3], cv->Signature.Data4[4], cv->Signature.Data4[5],
			cv->Signature.Data4[6], cv->Signature.Data4[7], cv->Age);
		std::string file((const char*)cv->PdbFileName, entry->SizeOfData - FIELD_OFFSET(CV_INFO_PDB70, PdbFileName));
		_pdbFile = Helpers::StringToWstring(file).c_str();
		memcpy(&_pdbValidation.guid, &cv->Signature, sizeof(GUID));
		_pdbValidation.signature = 0;
		_pdbValidation.age = cv->Age;
	}
	_path = _pdbFile + L"\\" + _pdbSignature;
	return true;
}

downslib_error SymbolFileInfo::Download(std::string url, std::wstring fileName, std::string userAgent,
    unsigned int timeout, downslib_cb cb, void* userdata) {
    std::ofstream fileStream(fileName, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!fileStream) {
        return downslib_error::incomplete;
    }

    using tcp = asio::ip::tcp;
    namespace ssl = asio::ssl;

    try {
        asio::io_context ioc;
        ssl::context ctx(ssl::context::sslv23_client);

        int redirect_count = 0;
        const int max_redirects = 5;

        while (redirect_count < max_redirects) {
            std::string protocol, host, path;

            if (url.substr(0, 8) == "https://") {
                protocol = "https";
                url = url.substr(8);
            }
            else if (url.substr(0, 7) == "http://") {
                protocol = "http";
                url = url.substr(7);
            }
            else {
                throw std::runtime_error("Invalid URL protocol");
            }

            size_t path_pos = url.find('/');
            host = (path_pos == std::string::npos) ? url : url.substr(0, path_pos);
            path = (path_pos == std::string::npos) ? "/" : url.substr(path_pos);

            tcp::resolver resolver(ioc);
            auto const results = resolver.resolve(host, protocol);

            std::string request = "GET " + path + " HTTP/1.1\r\n"
                "Host: " + host + "\r\n"
                "User-Agent: " + userAgent + "\r\n"
                "Accept: */*\r\n"
                "Connection: close\r\n\r\n";

            if (protocol == "https") {
                ssl::stream<tcp::socket> stream(ioc, ctx);
                SSL_set_tlsext_host_name(stream.native_handle(), host.c_str());

                asio::connect(stream.next_layer(), results.begin(), results.end());
                stream.handshake(ssl::stream_base::client);
                asio::write(stream, asio::buffer(request));

                asio::streambuf response;
                asio::error_code header_ec;
                asio::read_until(stream, response, "\r\n\r\n", header_ec);
                if (header_ec) {
                    throw asio::system_error(header_ec);
                }

                std::istream response_stream(&response);
                std::string http_version;
                unsigned int status_code;
                std::string status_message;

                response_stream >> http_version >> status_code;
                std::getline(response_stream, status_message);

                if (status_code == 301 || status_code == 302 || status_code == 307 || status_code == 308) {
                    std::string header;
                    while (std::getline(response_stream, header) && header != "\r") {
                        if (header.substr(0, 9) == "Location:") {
                            std::string new_url = header.substr(10);
                            new_url.erase(0, new_url.find_first_not_of(" \t"));
                            new_url.erase(new_url.find_last_not_of(" \r\n") + 1);

                            if (new_url[0] == '/') {
                                new_url = protocol + "://" + host + new_url;
                            }
                            url = new_url;
                            redirect_count++;
                            goto continue_redirect;
                        }
                    }
                }

                if (status_code != 200) {
                    return downslib_error::incomplete;
                }

                // Skip remaining headers
                std::string header;
                while (std::getline(response_stream, header) && header != "\r");

                // Write any data that was already read along with the headers
                if (response.size() > 0) {
                    fileStream << &response;
                }

                // Read the response body in chunks
                const size_t chunk_size = 8192;
                std::vector<char> buffer(chunk_size);
                asio::error_code ec;
                size_t bytes_transferred = 0;

                while (true) {
                    bytes_transferred = stream.read_some(asio::buffer(buffer), ec);

                    if (ec) {
                        // Check if this is a normal end of stream
                        if (ec == asio::error::eof ||
                            ec.value() == 1 ||  // stream_truncated
                            ec == asio::ssl::error::stream_truncated) {
                            break;  // Normal termination
                        }
                        throw asio::system_error(ec);
                    }

                    if (bytes_transferred > 0) {
                        fileStream.write(buffer.data(), bytes_transferred);
                        if (!fileStream) {
                            throw std::runtime_error("Failed to write to file");
                        }
                    }

                    if (bytes_transferred == 0) {
                        break;
                    }
                }
                break;
            }
            else {  // HTTP
                tcp::socket socket(ioc);
                asio::connect(socket, results.begin(), results.end());
                asio::write(socket, asio::buffer(request));

                asio::streambuf response;
                asio::error_code header_ec;
                asio::read_until(socket, response, "\r\n\r\n", header_ec);
                if (header_ec) {
                    throw asio::system_error(header_ec);
                }

                std::istream response_stream(&response);
                std::string http_version;
                unsigned int status_code;
                std::string status_message;

                response_stream >> http_version >> status_code;
                std::getline(response_stream, status_message);

                if (status_code == 301 || status_code == 302 || status_code == 307 || status_code == 308) {
                    std::string header;
                    while (std::getline(response_stream, header) && header != "\r") {
                        if (header.substr(0, 9) == "Location:") {
                            std::string new_url = header.substr(10);
                            new_url.erase(0, new_url.find_first_not_of(" \t"));
                            new_url.erase(new_url.find_last_not_of(" \r\n") + 1);

                            if (new_url[0] == '/') {
                                new_url = protocol + "://" + host + new_url;
                            }
                            url = new_url;
                            redirect_count++;
                            goto continue_redirect;
                        }
                    }
                }

                if (status_code != 200) {
                    return downslib_error::incomplete;
                }

                // Skip remaining headers
                std::string header;
                while (std::getline(response_stream, header) && header != "\r");

                // Write any data that was already read along with the headers
                if (response.size() > 0) {
                    fileStream << &response;
                }

                // Read the response body in chunks
                const size_t chunk_size = 8192;
                std::vector<char> buffer(chunk_size);
                asio::error_code ec;
                size_t bytes_transferred = 0;

                while (true) {
                    bytes_transferred = socket.read_some(asio::buffer(buffer), ec);

                    if (ec) {
                        if (ec == asio::error::eof || ec.value() == 1) {
                            break;  // Normal termination
                        }
                        throw asio::system_error(ec);
                    }

                    if (bytes_transferred > 0) {
                        fileStream.write(buffer.data(), bytes_transferred);
                        if (!fileStream) {
                            throw std::runtime_error("Failed to write to file");
                        }
                    }

                    if (bytes_transferred == 0) {
                        break;
                    }
                }
                break;
            }

        continue_redirect:
            continue;
        }

        if (redirect_count >= max_redirects) {
            throw std::runtime_error("Too many redirects");
        }
    }
    catch (std::exception&) {
        fileStream.close();
        return downslib_error::incomplete;
    }

    if (cb) {
        cb(userdata, 100, 100);
    }
    fileStream.close();
    return downslib_error::ok;
}

SymbolFileInfo::SymbolFileInfo() {
}

SymbolFileInfo::~SymbolFileInfo() {
}