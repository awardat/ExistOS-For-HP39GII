#ifndef KHI_I18N_H
#define KHI_I18N_H

// KhiCAS 语言系统
// lang=0: English, lang=1: Français, lang=2: 中文
// 与 ExistOS 语言系统桥接: ExistOS 0=EN, 1=CN → KhiCAS 0=EN, 2=CN

#ifdef __cplusplus
extern "C" {
#endif

extern int lang;

// 三语言选择器：lang==2 中文, lang==1 法语, 其他 英语
#define TR(en, fr, cn) (lang == 2 ? (cn) : (lang == 1 ? (fr) : (en)))
// 双语言选择器：lang!=0 中文, 否则 英语（用于只保留中英的场景）
#define TR2(en, cn) (lang ? (cn) : (en))

#ifdef __cplusplus
}
#endif

#endif // KHI_I18N_H
