#include "CookiesSession.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <ctime>
#include <unistd.h>

static int g_passed = 0;
static int g_failed = 0;

static void printResult(const std::string &name, bool ok)
{
    if (ok)
    {
        std::cout << "[PASS] " << name << std::endl;
        ++g_passed;
    }
    else
    {
        std::cout << "[FAIL] " << name << std::endl;
        ++g_failed;
    }
}

static HttpRequest makeRequest(const std::string &cookie)
{
    HttpRequest request;
    std::string raw;

    raw = "GET / HTTP/1.1\r\n";
    raw += "Host: localhost\r\n";

    if (!cookie.empty())
        raw += "Cookie: " + cookie + "\r\n";

    raw += "\r\n";

    request.parse(raw);
    return request;
}

static std::string getResponseHeader(const HttpResponse &response,
                                     const std::string &headerName)
{
    std::string raw = response.toString();
    std::istringstream stream(raw);
    std::string line;
    std::string prefix = headerName + ":";

    while (std::getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if (line.size() >= prefix.size()
            && line.compare(0, prefix.size(), prefix) == 0)
        {
            std::string value = line.substr(prefix.size());

            while (!value.empty()
                   && (value[0] == ' ' || value[0] == '\t'))
                value.erase(0, 1);

            return value;
        }
    }

    return "";
}

static bool hasHeader(const HttpResponse &response,
                      const std::string &headerName)
{
    return !getResponseHeader(response, headerName).empty();
}

static std::string extractCookieSessionId(const std::string &setCookie)
{
    const std::string key = "session_id=";
    std::size_t pos = setCookie.find(key);

    if (pos == std::string::npos)
        return "";

    pos += key.size();

    std::size_t end = setCookie.find(';', pos);

    if (end == std::string::npos)
        return setCookie.substr(pos);

    return setCookie.substr(pos, end - pos);
}

static bool isHexString(const std::string &value)
{
    if (value.empty())
        return false;

    for (std::size_t i = 0; i < value.size(); ++i)
    {
        char c = value[i];

        if (!((c >= '0' && c <= '9')
              || (c >= 'a' && c <= 'f')
              || (c >= 'A' && c <= 'F')))
            return false;
    }

    return true;
}

/*
 * 1. New request without cookie must create a session.
 */
static void testCreatesNewSession()
{
    CookiesSession sessions;
    HttpRequest request = makeRequest("");
    HttpResponse response;

    std::string id = sessions.getCreateSession(request, response);

    bool ok = !id.empty();

    printResult("new request creates session", ok);
}

/*
 * 2. Session ID must be exactly 32 hexadecimal characters.
 *    16 random bytes => 32 hex chars.
 */
static void testSessionIdFormat()
{
    CookiesSession sessions;
    HttpRequest request = makeRequest("");
    HttpResponse response;

    std::string id = sessions.getCreateSession(request, response);

    bool ok = (id.size() == 32 && isHexString(id));

    printResult("session id is 32 hexadecimal characters", ok);
}

/*
 * 3. Newly created session must be valid.
 */
static void testNewSessionIsValid()
{
    CookiesSession sessions;
    HttpRequest request = makeRequest("");
    HttpResponse response;

    std::string id = sessions.getCreateSession(request, response);

    bool ok = sessions.isValid(id);

    printResult("new session is immediately valid", ok);
}

/*
 * 4. Existing valid session must be reused.
 */
static void testValidSessionIsReused()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;

    std::string firstId =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest secondRequest =
        makeRequest("session_id=" + firstId);

    HttpResponse secondResponse;

    std::string secondId =
        sessions.getCreateSession(secondRequest, secondResponse);

    bool ok = (firstId == secondId);

    printResult("valid session is reused", ok);
}

/*
 * 5. Valid existing session should NOT emit Set-Cookie again.
 */
static void testValidSessionDoesNotResetCookie()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;

    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest secondRequest =
        makeRequest("session_id=" + id);

    HttpResponse secondResponse;

    sessions.getCreateSession(secondRequest, secondResponse);

    bool ok = !hasHeader(secondResponse, "Set-Cookie");

    printResult("valid session does not emit new Set-Cookie", ok);
}

/*
 * 6. Missing cookie must create a new session.
 */
static void testMissingCookieCreatesSession()
{
    CookiesSession sessions;

    HttpRequest request = makeRequest("");
    HttpResponse response;

    std::string id = sessions.getCreateSession(request, response);

    bool ok = !id.empty() && sessions.isValid(id);

    printResult("missing cookie creates session", ok);
}

/*
 * 7. Unknown session ID must create a new session.
 */
static void testUnknownSessionCreatesNewSession()
{
    CookiesSession sessions;

    HttpRequest request =
        makeRequest("session_id=does-not-exist");

    HttpResponse response;

    std::string id = sessions.getCreateSession(request, response);

    bool ok = !id.empty()
              && id != "does-not-exist"
              && sessions.isValid(id);

    printResult("unknown session id creates new session", ok);
}

/*
 * 8. Empty session_id must create a new session.
 */
static void testEmptySessionIdCreatesNewSession()
{
    CookiesSession sessions;

    HttpRequest request =
        makeRequest("session_id=");

    HttpResponse response;

    std::string id = sessions.getCreateSession(request, response);

    bool ok = !id.empty() && sessions.isValid(id);

    printResult("empty session id creates new session", ok);
}

/*
 * 9. Cookie with additional attributes must still extract ID.
 */
static void testCookieWithAttributes()
{
    CookiesSession sessions;

    HttpRequest request =
        makeRequest("session_id=abcdef1234567890; theme=dark; foo=bar");

    HttpResponse response;

    std::string id = sessions.getCreateSession(request, response);

    /*
     * Since the supplied ID isn't a known session, a new one is expected.
     * The important part here is that parsing must not accidentally include
     * "; theme=dark" in the session ID.
     */
    bool ok = (id != "abcdef1234567890; theme=dark; foo=bar")
              && (id.find(';') == std::string::npos);

    printResult("cookie attributes are not included in session id", ok);
}

/*
 * 10. Session ID can appear after another cookie.
 */
static void testSessionCookieAfterAnotherCookie()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;

    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest secondRequest =
        makeRequest("theme=dark; session_id=" + id + "; foo=bar");

    HttpResponse secondResponse;

    std::string returnedId =
        sessions.getCreateSession(secondRequest, secondResponse);

    bool ok = (returnedId == id);

    printResult("session_id after another cookie is reused", ok);
}

/*
 * 11. Session ID at the end of Cookie header.
 */
static void testSessionCookieAtEnd()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;

    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest secondRequest =
        makeRequest("theme=dark; session_id=" + id);

    HttpResponse secondResponse;

    std::string returnedId =
        sessions.getCreateSession(secondRequest, secondResponse);

    bool ok = (returnedId == id);

    printResult("session_id at end of cookie is reused", ok);
}

/*
 * 12. Set-Cookie must contain required attributes.
 */
static void testSetCookieAttributes()
{
    CookiesSession sessions;
    HttpRequest request = makeRequest("");
    HttpResponse response;

    sessions.getCreateSession(request, response);

    std::string cookie = getResponseHeader(response, "Set-Cookie");

    bool ok = cookie.find("session_id=") != std::string::npos
              && cookie.find("Path=/") != std::string::npos
              && cookie.find("HttpOnly") != std::string::npos
              && cookie.find("Max-Age=3600") != std::string::npos;

    printResult("Set-Cookie contains required attributes", ok);
}

/*
 * 13. Set-Cookie session ID must match returned session ID.
 */
static void testSetCookieMatchesReturnedId()
{
    CookiesSession sessions;
    HttpRequest request = makeRequest("");
    HttpResponse response;

    std::string id =
        sessions.getCreateSession(request, response);

    std::string cookie =
        getResponseHeader(response, "Set-Cookie");

    std::string cookieId =
        extractCookieSessionId(cookie);

    bool ok = (id == cookieId);

    printResult("Set-Cookie contains returned session id", ok);
}

/*
 * 14. Session ID must not contain cookie separators.
 */
static void testGeneratedIdContainsNoCookieSeparators()
{
    CookiesSession sessions;
    HttpRequest request = makeRequest("");
    HttpResponse response;

    std::string id =
        sessions.getCreateSession(request, response);

    bool ok = id.find(';') == std::string::npos
              && id.find('=') == std::string::npos
              && id.find(' ') == std::string::npos
              && id.find('\r') == std::string::npos
              && id.find('\n') == std::string::npos;

    printResult("generated session id is safe for Cookie syntax", ok);
}

/*
 * 15. Two newly created sessions must have different IDs.
 */
static void testTwoSessionsAreDifferent()
{
    CookiesSession sessions;

    HttpRequest request1 = makeRequest("");
    HttpResponse response1;

    HttpRequest request2 = makeRequest("");
    HttpResponse response2;

    std::string id1 =
        sessions.getCreateSession(request1, response1);

    std::string id2 =
        sessions.getCreateSession(request2, response2);

    bool ok = !id1.empty()
              && !id2.empty()
              && id1 != id2;

    printResult("two new sessions receive different ids", ok);
}

/*
 * 16. Generate several sessions and verify uniqueness.
 */
static void testManySessionsAreUnique()
{
    CookiesSession sessions;
    std::string ids[20];
    bool ok = true;

    for (int i = 0; i < 20; ++i)
    {
        HttpRequest request = makeRequest("");
        HttpResponse response;

        ids[i] =
            sessions.getCreateSession(request, response);

        if (ids[i].empty())
            ok = false;

        for (int j = 0; j < i; ++j)
        {
            if (ids[i] == ids[j])
                ok = false;
        }
    }

    printResult("20 generated sessions are unique", ok);
}

/*
 * 17. Invalid session must be removed from the internal session map
 *     after expiration. We cannot directly access the map, but calling
 *     isValid twice should consistently return false.
 *
 *     NOTE: Current class does not expose a way to manipulate expiration,
 *     so this test mainly documents expected behavior and cannot force
 *     expiration without waiting 1 hour.
 */
static void testInvalidSessionRemainsInvalid()
{
    CookiesSession sessions;

    bool ok = !sessions.isValid("")
              && !sessions.isValid("nonexistent");

    printResult("invalid sessions remain invalid", ok);
}

/*
 * 18. Cookie header containing a different cookie first must not
 *     accidentally use that cookie's value.
 */
static void testDoesNotUseWrongCookie()
{
    CookiesSession sessions;

    HttpRequest request =
        makeRequest("user_id=wrong-value; theme=dark");

    HttpResponse response;

    std::string id =
        sessions.getCreateSession(request, response);

    bool ok = !id.empty()
              && id != "wrong-value"
              && id != "dark";

    printResult("other cookies are not mistaken for session id", ok);
}

/*
 * 19. Prefix collision test.
 *
 * session_id_extra must NOT be treated as session_id.
 */
static void testSessionIdPrefixCollision()
{
    CookiesSession sessions;

    HttpRequest request =
        makeRequest("session_id_extra=abcdef");

    HttpResponse response;

    std::string id =
        sessions.getCreateSession(request, response);

    bool ok = !id.empty()
              && id != "abcdef";

    printResult("session_id prefix collision is rejected", ok);
}

/*
 * 20. Similar cookie name after another cookie.
 */
static void testSessionIdPrefixCollisionAfterCookie()
{
    CookiesSession sessions;

    HttpRequest request =
        makeRequest("foo=bar; session_id_extra=abcdef");

    HttpResponse response;

    std::string id =
        sessions.getCreateSession(request, response);

    bool ok = !id.empty()
              && id != "abcdef";

    printResult("session_id prefix collision after another cookie is rejected", ok);
}

/*
 * 21. Cookie with spaces around semicolon.
 */
static void testCookieWithSpaces()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;

    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest secondRequest =
        makeRequest("foo=bar; session_id=" + id + "; theme=dark");

    HttpResponse secondResponse;

    std::string returnedId =
        sessions.getCreateSession(secondRequest, secondResponse);

    bool ok = (returnedId == id);

    printResult("normal cookie spacing works", ok);
}

/*
 * 22. Cookie header with only session_id.
 */
static void testOnlySessionCookie()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;

    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest secondRequest =
        makeRequest("session_id=" + id);

    HttpResponse secondResponse;

    std::string returnedId =
        sessions.getCreateSession(secondRequest, secondResponse);

    bool ok = (returnedId == id);

    printResult("cookie containing only session_id works", ok);
}

/*
 * 23. Multiple session_id cookies.
 *
 * Current implementation uses the first occurrence.
 * This test documents that behavior.
 */
static void testMultipleSessionIdCookies()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;

    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest secondRequest =
        makeRequest("session_id=" + id + "; session_id=other");

    HttpResponse secondResponse;

    std::string returnedId =
        sessions.getCreateSession(secondRequest, secondResponse);

    bool ok = (returnedId == id);

    printResult("first session_id cookie is used", ok);
}

/*
 * 24. A valid session should stay valid across multiple requests.
 */
static void testSessionPersistsAcrossMultipleRequests()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;

    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    bool ok = true;

    for (int i = 0; i < 10; ++i)
    {
        HttpRequest request =
            makeRequest("session_id=" + id);

        HttpResponse response;

        std::string returnedId =
            sessions.getCreateSession(request, response);

        if (returnedId != id)
            ok = false;

        if (!sessions.isValid(id))
            ok = false;
    }

    printResult("session persists across multiple requests", ok);
}

/*
 * 25. Different CookiesSession instances must not share sessions.
 */
static void testSessionsAreInstanceLocal()
{
    CookiesSession sessions1;
    CookiesSession sessions2;

    HttpRequest request1 = makeRequest("");
    HttpResponse response1;

    std::string id =
        sessions1.getCreateSession(request1, response1);

    bool ok = sessions1.isValid(id)
              && !sessions2.isValid(id);

    printResult("session storage is instance-local", ok);
}

static void testUppercaseSessionIdCookieName()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;
    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest request =
        makeRequest("SESSION_ID=" + id);

    HttpResponse response;

    std::string returnedId =
        sessions.getCreateSession(request, response);

    bool ok = returnedId != id;

    printResult("uppercase SESSION_ID is not treated as session_id", ok);
}

static void testMixedCaseSessionIdCookieName()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;
    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest request =
        makeRequest("Session_Id=" + id);

    HttpResponse response;

    std::string returnedId =
        sessions.getCreateSession(request, response);

    bool ok = returnedId != id;

    printResult("mixed-case Session_Id is not treated as session_id", ok);
}

static void testSpaceBeforeEquals()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;
    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest request =
        makeRequest("session_id =" + id);

    HttpResponse response;

    std::string returnedId =
        sessions.getCreateSession(request, response);

    bool ok = returnedId != id;

    printResult("space before equals is not accepted", ok);
}

static void testSpaceAfterEquals()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;
    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest request =
        makeRequest("session_id= " + id);

    HttpResponse response;

    std::string returnedId =
        sessions.getCreateSession(request, response);

    bool ok = returnedId != id;

    printResult("leading space in session id is not accepted", ok);
}

static void testTrailingSpaceInSessionId()
{
    CookiesSession sessions;

    // 1. Düzgün bir oturum oluştur
    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;
    std::string validId = sessions.getCreateSession(firstRequest, firstResponse);

    // 2. RFC 7230 uyarınca sondaki boşluk (OWS) HTTP parser tarafından temizlenir.
    // Dolayısıyla "session_id=ID " isteği, parser'dan "session_id=ID" olarak geçer 
    // ve mevcut geçerli oturumla eşleşir.
    HttpRequest requestWithOws = makeRequest("session_id=" + validId + " ");
    HttpResponse responseOws;
    std::string returnedIdOws = sessions.getCreateSession(requestWithOws, responseOws);

    // HTTP Parser OWS'i sildiği için oturumlar EŞİT olmalıdır.
    bool owsTrimmedCorrectly = (returnedIdOws == validId);
    printResult("RFC 7230: trailing header OWS is trimmed correctly", owsTrimmedCorrectly);

    // 3. Tırnak içine alınmış boşluklu geçersiz session ID (RFC 6265 cookie değeri)
    // Tırnak içindeki boşluk OWS değildir, geçerli cookie değerinin parçasıdır.
    // Bu durumda sunucu bu ID'yi bulamamalı ve YENİ bir ID üretmelidir.
    HttpRequest requestQuoted = makeRequest("session_id=\"" + validId + " \"");
    HttpResponse responseQuoted;
    std::string returnedIdQuoted = sessions.getCreateSession(requestQuoted, responseQuoted);

    bool invalidSessionRejected = (returnedIdQuoted != validId);
    printResult("RFC 6265: quoted space in session id creates new session", invalidSessionRejected);
}

static void testCookieWithoutSpaceAfterSemicolon()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;
    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest request =
        makeRequest("foo=bar;session_id=" + id);

    HttpResponse response;

    std::string returnedId =
        sessions.getCreateSession(request, response);

    bool ok = returnedId == id;

    printResult("cookie without space after semicolon works", ok);
}

static void testDoubleSemicolonBeforeSession()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;
    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest request =
        makeRequest("foo=bar;;session_id=" + id);

    HttpResponse response;

    std::string returnedId =
        sessions.getCreateSession(request, response);

    bool ok = returnedId == id;

    printResult("double semicolon before session works", ok);
}

static void testLeadingSemicolonBeforeSession()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;
    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest request =
        makeRequest(";session_id=" + id);

    HttpResponse response;

    std::string returnedId =
        sessions.getCreateSession(request, response);

    bool ok = returnedId == id;

    printResult("leading semicolon before session works", ok);
}

static void testSessionIdWithoutEquals()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;
    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest request =
        makeRequest("session_id");

    HttpResponse response;

    std::string returnedId =
        sessions.getCreateSession(request, response);

    bool ok = !returnedId.empty()
              && returnedId != id
              && sessions.isValid(returnedId);

    printResult("session_id without equals creates new session", ok);
}

static void testDoubleEqualsInSessionId()
{
    CookiesSession sessions;

    HttpRequest request =
        makeRequest("session_id==abc");

    HttpResponse response;

    std::string id =
        sessions.getCreateSession(request, response);

    bool ok = !id.empty()
              && id != "=abc"
              && sessions.isValid(id);

    printResult("double equals creates new session", ok);
}

static void testEqualsInsideSessionId()
{
    CookiesSession sessions;

    HttpRequest request =
        makeRequest("session_id=abc=def");

    HttpResponse response;

    std::string id =
        sessions.getCreateSession(request, response);

    bool ok = !id.empty()
              && id != "abc=def"
              && sessions.isValid(id);

    printResult("equals inside supplied session id is rejected", ok);
}

static void testPrefixWithLeadingCharacter()
{
    CookiesSession sessions;

    HttpRequest request =
        makeRequest("xsession_id=abcdef");

    HttpResponse response;

    std::string id =
        sessions.getCreateSession(request, response);

    bool ok = !id.empty()
              && id != "abcdef"
              && sessions.isValid(id);

    printResult("xsession_id is not treated as session_id", ok);
}

static void testInvalidShortSessionId()
{
    CookiesSession sessions;

    bool ok = !sessions.isValid("abc");

    printResult("short session id is invalid", ok);
}

static void testInvalidLongSessionId()
{
    CookiesSession sessions;

    std::string id(33, 'a');

    bool ok = !sessions.isValid(id);

    printResult("long session id is invalid", ok);
}

static void testInvalidNonHexSessionId()
{
    CookiesSession sessions;

    bool ok = !sessions.isValid("0123456789abcdef0123456789abcdeg");

    printResult("non-hex session id is invalid", ok);
}

static void testInvalidSessionEmitsNewCookie()
{
    CookiesSession sessions;

    HttpRequest request =
        makeRequest("session_id=invalid-session");

    HttpResponse response;

    std::string id =
        sessions.getCreateSession(request, response);

    bool ok = !id.empty()
              && hasHeader(response, "Set-Cookie");

    printResult("invalid session emits new Set-Cookie", ok);
}

static void testInvalidSessionGetsDifferentId()
{
    CookiesSession sessions;

    const std::string invalidId = "invalid-session";

    HttpRequest request =
        makeRequest("session_id=" + invalidId);

    HttpResponse response;

    std::string id =
        sessions.getCreateSession(request, response);

    bool ok = !id.empty()
              && id != invalidId;

    printResult("invalid session gets a different id", ok);
}

static void testSessionsRemainIndependent()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;

    std::string firstId =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest secondRequest =
        makeRequest("session_id=invalid");

    HttpResponse secondResponse;

    std::string secondId =
        sessions.getCreateSession(secondRequest, secondResponse);

    bool ok = firstId != secondId
              && sessions.isValid(firstId)
              && sessions.isValid(secondId);

    printResult("creating replacement session does not invalidate old session", ok);
}

static void testExistingSetCookieIsReplacedForNewSession()
{
    CookiesSession sessions;

    HttpRequest request =
        makeRequest("");

    HttpResponse response;

    response.setHeader("Set-Cookie", "old=value");

    std::string id =
        sessions.getCreateSession(request, response);

    std::string cookie =
        getResponseHeader(response, "Set-Cookie");

    bool ok = !id.empty()
              && cookie != "old=value"
              && cookie.find("session_id=") != std::string::npos;

    printResult("new session replaces existing Set-Cookie", ok);
}

static void testExistingSetCookieWithValidSession()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;

    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest secondRequest =
        makeRequest("session_id=" + id);

    HttpResponse secondResponse;

    secondResponse.setHeader("Set-Cookie", "old=value");

    std::string returnedId =
        sessions.getCreateSession(secondRequest, secondResponse);

    std::string cookie =
        getResponseHeader(secondResponse, "Set-Cookie");

    /*
     * Current implementation does not touch Set-Cookie for a valid
     * session, so the existing header remains.
     */
    bool ok = returnedId == id
              && cookie == "old=value";

    printResult("valid session does not overwrite existing Set-Cookie", ok);
}

static void testCookieWithEmptyOtherValues()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;

    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest secondRequest =
        makeRequest("foo=;bar=;session_id=" + id + ";baz=");

    HttpResponse secondResponse;

    std::string returnedId =
        sessions.getCreateSession(secondRequest, secondResponse);

    bool ok = returnedId == id;

    printResult("empty neighboring cookie values do not break session parsing", ok);
}

static void testSessionIdAtVeryEnd()
{
    CookiesSession sessions;

    HttpRequest firstRequest = makeRequest("");
    HttpResponse firstResponse;

    std::string id =
        sessions.getCreateSession(firstRequest, firstResponse);

    HttpRequest secondRequest =
        makeRequest("foo=bar;session_id=" + id);

    HttpResponse secondResponse;

    std::string returnedId =
        sessions.getCreateSession(secondRequest, secondResponse);

    bool ok = returnedId == id
              && !hasHeader(secondResponse, "Set-Cookie");

    printResult("session id at exact end of cookie is reused", ok);
}

int main()
{
    testCreatesNewSession();
    testSessionIdFormat();
    testNewSessionIsValid();
    testValidSessionIsReused();
    testValidSessionDoesNotResetCookie();
    testMissingCookieCreatesSession();
    testUnknownSessionCreatesNewSession();
    testEmptySessionIdCreatesNewSession();
    testCookieWithAttributes();
    testSessionCookieAfterAnotherCookie();
    testSessionCookieAtEnd();
    testSetCookieAttributes();
    testSetCookieMatchesReturnedId();
    testGeneratedIdContainsNoCookieSeparators();
    testTwoSessionsAreDifferent();
    testManySessionsAreUnique();
    testInvalidSessionRemainsInvalid();
    testDoesNotUseWrongCookie();
    testSessionIdPrefixCollision();
    testSessionIdPrefixCollisionAfterCookie();
    testCookieWithSpaces();
    testOnlySessionCookie();
    testMultipleSessionIdCookies();
    testSessionPersistsAcrossMultipleRequests();
    testSessionsAreInstanceLocal();
	    testUppercaseSessionIdCookieName();
    testMixedCaseSessionIdCookieName();
    testSpaceBeforeEquals();
    testSpaceAfterEquals();
    testTrailingSpaceInSessionId();
    testCookieWithoutSpaceAfterSemicolon();
    testDoubleSemicolonBeforeSession();
    testLeadingSemicolonBeforeSession();
    testSessionIdWithoutEquals();
    testDoubleEqualsInSessionId();
    testEqualsInsideSessionId();
    testPrefixWithLeadingCharacter();
    testInvalidShortSessionId();
    testInvalidLongSessionId();
    testInvalidNonHexSessionId();
    testInvalidSessionEmitsNewCookie();
    testInvalidSessionGetsDifferentId();
    testSessionsRemainIndependent();
    testExistingSetCookieIsReplacedForNewSession();
    testExistingSetCookieWithValidSession();
    testCookieWithEmptyOtherValues();
    testSessionIdAtVeryEnd();

    std::cout << std::endl;
    std::cout << "Passed: " << g_passed << std::endl;
    std::cout << "Failed: " << g_failed << std::endl;

    return (g_failed == 0 ? 0 : 1);
}