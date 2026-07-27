using System;
using System.Collections.Generic;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.Networking;

namespace TankBattle.Network
{
    /// <summary>
    /// HTTP login flow matching demo/client: /login → /createRole → /enterGame.
    /// </summary>
    public static class LoginHttpClient
    {
        [Serializable]
        public class RoleInfo
        {
            public uint serverId;
            public ulong roleId;
        }

        public class LoginAccount
        {
            public ulong UserId;
            public string Token;
            public ulong RoleId;
            public List<RoleInfo> Roles = new List<RoleInfo>();
        }

        public class EnterGameInfo
        {
            public uint GateId;
            public string Host;
            public int Port;
            public string GToken;
        }

        public static async Task<LoginAccount> LoginAsync(string loginBaseUrl, string openId)
        {
            string body = "openid=" + UnityWebRequest.EscapeURL(openId);
            string json = await PostFormAsync(TrimSlash(loginBaseUrl) + "/login", body);
            var root = JsonUtility.FromJson<LoginResponseDto>(WrapRoles(json));
            if (root == null || root.retCode != 0)
                throw new Exception("login failed: " + json);

            var account = new LoginAccount
            {
                UserId = root.userId,
                Token = root.token ?? ""
            };

            // Parse roles array manually (Unity JsonUtility is weak on arrays in nested objects)
            ParseRoles(json, account);
            if (account.Roles.Count > 0)
                account.RoleId = account.Roles[0].roleId;
            return account;
        }

        public static async Task<ulong> CreateRoleAsync(
            string loginBaseUrl, ulong userId, string token, uint serverId, string name)
        {
            string body =
                "userId=" + userId +
                "&token=" + UnityWebRequest.EscapeURL(token) +
                "&serverId=" + serverId +
                "&name=" + UnityWebRequest.EscapeURL(name);
            string json = await PostFormAsync(TrimSlash(loginBaseUrl) + "/createRole", body);
            if (!TryGetInt(json, "retCode", out int ret) || ret != 0)
                throw new Exception("createRole failed: " + json);

            if (!TryGetULongInRole(json, out ulong roleId) || roleId == 0)
                throw new Exception("createRole missing roleId: " + json);
            return roleId;
        }

        public static async Task<EnterGameInfo> EnterGameAsync(
            string loginBaseUrl, ulong userId, ulong roleId, string token, uint serverId)
        {
            string body =
                "userId=" + userId +
                "&roleId=" + roleId +
                "&token=" + UnityWebRequest.EscapeURL(token) +
                "&serverId=" + serverId;
            string json = await PostFormAsync(TrimSlash(loginBaseUrl) + "/enterGame", body);
            var dto = JsonUtility.FromJson<EnterGameResponseDto>(json);
            if (dto == null || dto.retCode != 0)
                throw new Exception("enterGame failed: " + json);
            if (string.IsNullOrEmpty(dto.gToken) || string.IsNullOrEmpty(dto.host))
                throw new Exception("enterGame missing gate info: " + json);

            return new EnterGameInfo
            {
                GateId = dto.gateId,
                Host = dto.host,
                Port = (int)dto.port,
                GToken = dto.gToken
            };
        }

        static async Task<string> PostFormAsync(string url, string body)
        {
            byte[] raw = Encoding.UTF8.GetBytes(body);
            using (var req = new UnityWebRequest(url, UnityWebRequest.kHttpVerbPOST))
            {
                req.uploadHandler = new UploadHandlerRaw(raw);
                req.downloadHandler = new DownloadHandlerBuffer();
                req.SetRequestHeader("Content-Type", "application/x-www-form-urlencoded");
                req.timeout = 10;
                var op = req.SendWebRequest();
                while (!op.isDone)
                    await Task.Yield();

#if UNITY_2020_2_OR_NEWER
                if (req.result != UnityWebRequest.Result.Success)
#else
                if (req.isNetworkError || req.isHttpError)
#endif
                {
                    throw new Exception("HTTP error " + url + ": " + req.error);
                }
                return req.downloadHandler.text ?? "";
            }
        }

        static string TrimSlash(string url)
        {
            if (string.IsNullOrEmpty(url)) return "";
            return url.TrimEnd('/');
        }

        static void ParseRoles(string json, LoginAccount account)
        {
            int idx = json.IndexOf("\"roles\"", StringComparison.Ordinal);
            if (idx < 0) return;
            int arrStart = json.IndexOf('[', idx);
            int arrEnd = json.IndexOf(']', arrStart + 1);
            if (arrStart < 0 || arrEnd < 0) return;
            string arr = json.Substring(arrStart, arrEnd - arrStart + 1);
            // naive object split
            int pos = 0;
            while (true)
            {
                int o0 = arr.IndexOf('{', pos);
                if (o0 < 0) break;
                int o1 = arr.IndexOf('}', o0 + 1);
                if (o1 < 0) break;
                string obj = arr.Substring(o0, o1 - o0 + 1);
                var role = JsonUtility.FromJson<RoleInfo>(obj);
                if (role != null && role.roleId != 0)
                    account.Roles.Add(role);
                pos = o1 + 1;
            }
        }

        static bool TryGetInt(string json, string key, out int value)
        {
            value = 0;
            string pattern = "\"" + key + "\"";
            int i = json.IndexOf(pattern, StringComparison.Ordinal);
            if (i < 0) return false;
            int colon = json.IndexOf(':', i + pattern.Length);
            if (colon < 0) return false;
            int end = colon + 1;
            while (end < json.Length && (char.IsDigit(json[end]) || json[end] == '-')) end++;
            return int.TryParse(json.Substring(colon + 1, end - colon - 1).Trim(), out value);
        }

        static bool TryGetULongInRole(string json, out ulong roleId)
        {
            roleId = 0;
            int roleObj = json.IndexOf("\"role\"", StringComparison.Ordinal);
            if (roleObj < 0) return false;
            int idKey = json.IndexOf("\"roleId\"", roleObj, StringComparison.Ordinal);
            if (idKey < 0) return false;
            int colon = json.IndexOf(':', idKey);
            if (colon < 0) return false;
            int end = colon + 1;
            while (end < json.Length && char.IsDigit(json[end])) end++;
            return ulong.TryParse(json.Substring(colon + 1, end - colon - 1).Trim(), out roleId);
        }

        // JsonUtility helpers — roles wrapped separately
        static string WrapRoles(string json) => json;

        [Serializable]
        class LoginResponseDto
        {
            public int retCode;
            public ulong userId;
            public string token;
        }

        [Serializable]
        class EnterGameResponseDto
        {
            public int retCode;
            public uint gateId;
            public string host;
            public uint port;
            public string gToken;
        }
    }
}
