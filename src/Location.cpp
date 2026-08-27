#include "Location.hpp"

Location::Location() // Default constructor
    : _autoindex(false),
      _autoindexSet(false),
      _upload(false)
{
}

Location::Location(const std::string &path) // Constructor with a specified path
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

bool Location::isMethodAllowed(const std::string &method) const // Check if a specific HTTP method is allowed for this location
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

void Location::addCgiExtension(const std::string &extension, const std::string &interpreterPath)
{
	_cgiExtensions[extension] = interpreterPath;
}

bool Location::isCgiExtension(const std::string &extension) const
{

	if (_cgiExtensions.find(extension) == _cgiExtensions.end())
		return (false);
	else
		return (true);
}

const std::string &Location::getCgiInterpreter (const std::string &extension) const
{
	static const std::string empty("");
	std::map<std::string, std::string>::const_iterator it = _cgiExtensions.find(extension);

	if (it == _cgiExtensions.end())
		return (empty);
	return (it->second);
}
