#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>

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

        const std::string &getPath() const;
        const std::string &getRoot() const;
        const std::string &getIndex() const;
        bool getAutoIndex() const;
        bool isAutoIndexSet() const;
        bool getUpload() const;
        const std::string &getUploadStore() const;

        bool isMethodAllowed(const std::string &method) const;
};

#endif