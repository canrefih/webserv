#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>
#include <map>
#include <set>

/*
Location class represents a specific location block in the server configuration.
It holds information about the path, root directory, index file, autoindex setting, upload settings, and allowed HTTP methods for that location.
*/

class Location
{
    private:
        std::string              _path;
        std::string              _root;
        std::string              _index;
        bool                     _autoindex;
        bool                     _autoindexSet;
        bool                     _upload;
        std::string              _uploadStore;
        std::vector<std::string> _allowedMethods;
		std::map<std::string, std::string> _cgiExtensions;
		std::set<std::string> _virtualCgiExtensions; // Extensions whose CGI runs even if the requested file does not exist
        int                      _returnCode;
        std::string              _returnPath;
        std::size_t              _clientMaxBodySize; // Overrides the server limit when _clientMaxBodySizeSet is true
        bool                     _clientMaxBodySizeSet;

    public:
        Location();
        Location(const std::string &path);
        ~Location();

        void setPath(const std::string &path);
        void setRoot(const std::string &root);
        void setIndex(const std::string &index);
        void setAutoIndex(bool value);
        void setUpload(bool value);
        void setUploadStore(const std::string &path);
        void addMethod(const std::string &method);

		/*CGI*/
		void addCgiExtension(const std::string &extension, const std::string &interpreterPath);
		bool isCgiExtension(const std::string &extension) const;
		const std::string &getCgiInterpreter(const std::string &extension) const;
		void setCgiVirtual(const std::string &extension);
		bool isCgiVirtual(const std::string &extension) const;
		/*end*/

        void setRedirection(int code, const std::string &path);
        int getReturnCode() const;
        const std::string &getReturnPath() const;

        const std::string &getPath() const;
        const std::string &getRoot() const;
        const std::string &getIndex() const;
        bool getAutoIndex() const;
        bool isAutoIndexSet() const;
        bool getUpload() const;
        const std::string &getUploadStore() const;

        void setClientMaxBodySize(std::size_t size);
        std::size_t getClientMaxBodySize() const;
        bool isClientMaxBodySizeSet() const;

        bool isMethodAllowed(const std::string &method) const; // Check if a specific HTTP method is allowed for this location
};

#endif
