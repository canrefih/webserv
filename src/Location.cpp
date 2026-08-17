#include "Location.hpp"

Location::Location()
    : _autoindex(false),
      _autoindexSet(false),
      _upload(false)
{
}

Location::Location(const std::string &path)
    : _path(path),
      _autoindex(false),
      _autoindexSet(false),
      _upload(false)
{
}

Location::~Location()
{
}

void Location::setPath(const std::string &path)
{
    _path = path;
}

void Location::setRoot(const std::string &root)
{
    _root = root;
}

void Location::setIndex(const std::string &index)
{
    _index = index;
}

void Location::setAutoIndex(bool value)
{
    _autoindex = value;
    _autoindexSet = true;
}

void Location::setUpload(bool value)
{
    _upload = value;
}

void Location::setUploadStore(const std::string &path)
{
    _uploadStore = path;
}

void Location::addMethod(const std::string &method)
{
    _allowedMethods.push_back(method);
}

const std::string &Location::getPath() const
{
    return _path;
}

const std::string &Location::getRoot() const
{
    return _root;
}

const std::string &Location::getIndex() const
{
    return _index;
}

bool Location::getAutoIndex() const
{
    return _autoindex;
}

bool Location::isAutoIndexSet() const
{
    return _autoindexSet;
}

bool Location::getUpload() const
{
    return _upload;
}

const std::string &Location::getUploadStore() const
{
    return _uploadStore;
}

bool Location::isMethodAllowed(const std::string &method) const
{
    if (_allowedMethods.empty())
        return true;

    std::vector<std::string>::const_iterator it;

    for (it = _allowedMethods.begin();
         it != _allowedMethods.end();
         ++it)
    {
        if (*it == method)
            return true;
    }

    return false;
}