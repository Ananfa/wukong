using UnityEditor;
using UnityEngine;

/// <summary>在编辑器中向 TagManager 写入自定义 Tag，避免运行时 obstacle.tag = "Obstacle" 等赋值失败。</summary>
public static class ProjectTagUtility
{
    [MenuItem("Tools/坦克大战/确保 Obstacle 与 TankBody 标签")]
    public static void EnsureGameplayTagsMenu()
    {
        EnsureTagExists("Obstacle");
        EnsureTagExists("TankBody");
        Debug.Log("ProjectTagUtility: 已尝试添加 Obstacle、TankBody 标签（若已存在则跳过）。");
    }

    public static void EnsureTagExists(string tag)
    {
        if (string.IsNullOrEmpty(tag) || ProjectTagExists(tag)) return;

        Object[] assets = AssetDatabase.LoadAllAssetsAtPath("ProjectSettings/TagManager.asset");
        if (assets == null || assets.Length == 0)
        {
            Debug.LogError("ProjectTagUtility: 未找到 ProjectSettings/TagManager.asset，无法添加标签: " + tag);
            return;
        }

        SerializedObject tagManager = new SerializedObject(assets[0]);
        SerializedProperty tagsProp = tagManager.FindProperty("tags");
        if (tagsProp == null) return;

        tagsProp.InsertArrayElementAtIndex(tagsProp.arraySize);
        SerializedProperty newTag = tagsProp.GetArrayElementAtIndex(tagsProp.arraySize - 1);
        newTag.stringValue = tag;
        tagManager.ApplyModifiedProperties();
        Debug.Log("ProjectTagUtility: 已添加标签: " + tag);
    }

    public static bool ProjectTagExists(string tag)
    {
        Object[] assets = AssetDatabase.LoadAllAssetsAtPath("ProjectSettings/TagManager.asset");
        if (assets == null || assets.Length == 0) return false;
        SerializedObject so = new SerializedObject(assets[0]);
        SerializedProperty tags = so.FindProperty("tags");
        if (tags == null) return false;
        for (int i = 0; i < tags.arraySize; i++)
        {
            if (tags.GetArrayElementAtIndex(i).stringValue == tag)
                return true;
        }
        return false;
    }
}
