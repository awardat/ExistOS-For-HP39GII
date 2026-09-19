# KhiCAS 内置函数参考（HP39GII）

> **自动生成**（`tools/gen_catalog_doc.py`）——数据来源为 KhiCAS 命令目录（与设备内「命令列表」一致），请勿手工编辑。
> 适用版本：build 139/140。生成日期：2026-09-19。

**怎么用**
- 在 Console 直接输入命令（函数名区分大小写）；参数用逗号分隔
- **MATH/F4** 打开命令列表按分类浏览；**Shift+0** 输入前缀自动补全
- 菜单内 **F6 / Shift+View** 显示当前高亮项的帮助
- 标记 **`*`** 的条目 = 仅 Xcas 模式（Python 兼容模式下不可用，共 95 条；该部分说明保留原文未汉化）

## 分类总览

| 分类 | 条目数 |
|------|--------|
| 代数 | 9 |
| 线性代数 | 6 |
| 微积分 | 14 |
| 算术，加密 crypto | 16 |
| 复数 | 6 |
| 曲线 | 13 |
| 多项式 | 17 |
| 概率 | 13 |
| 程序命令 | 53 |
| 实数 | 8 |
| 求解 | 8 |
| 统计 | 23 |
| 三角函数 | 9 |
| 列表 | 5 |
| 矩阵 | 18 |
| 程序 | 17 |
| 修改变量 | 1 |
| 物理常数 | 33 |
| 物理单位 | 47 |
| 几何 | 32 |
| 三维 | 23 |
| 海龟 | 24 |

> 分类次序与设备内命令列表一致（空分类不列出）。

## 1. 代数

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `cfactor(p) 复数分解` | 在复数域上分解因式。 | `cfactor(p)` | x^4-1 |
| `cpartfrac(p,x) 复部分分式` | p 关于 x 在复数域上的部分分式分解。 | `cpartfrac(p,x)` | 1/(x^4-1) |
| `factor(p,[x]) 因式分解` | 对多项式 p 因式分解（整数用 ifactor）。快捷键 p=>* | `factor(p,[x])` | x^4-1 |
| `mult_conjugate 共轭` | 乘以共轭（根式）。 | `mult_conjugate` | sqrt(2)-sqrt(3) |
| `partfrac(p,x) 部分分式` | 部分分式展开。快捷键 p=>+ | `partfrac(p,x)` | 1/(x^4-1) |
| `quote(x) 保持不求值` | 返回不求值的表达式 x。 | `quote(x)` | — |
| `ratnormal(x) 通分` | 通分合并为单一分母。 | `ratnormal(x)` | — |
| `simplify(expr) 化简` | 化简表达式。快捷键 expr=>/ | `simplify(expr)` | sin(3x)/sin(x) |
| `subst(a,b=c) 替换` | 把 a 中的 b 替换为 c。快捷键 a(b=c)。 | `subst(a,b=c)` | x^2,x=3 |

## 2. 线性代数

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `cross(u,v) 叉积` | 向量 u 与 v 的叉积。 | `cross(u,v)` | [1,2,3],[0,1,3] |
| `curl(u,vars) 旋度` | 向量 u 的旋度。 | `curl(u,vars)` | [2*x*y,x*z,y*z],[x,y,z] |
| `dot(a,b) 点积` | 两个向量的点积。快捷键 * | `dot(a,b)` | [1,2,3,4,5],[0,1,3,4,4] |
| `gauss(q) 二次型化简` | 二次型化简 | `gauss(q)` | x^2+x*y+x*z+y^2+z^2,[x,y,z] |
| `gramschmidt(M) 正交化` | Gram-Schmidt 正交化（行向量或线性无关向量组） | `gramschmidt(M)` | [[1,2,3],[4,5,6]] |
| `ranv(n,[loi,parametres]) 随机向量` | 随机向量。 | `ranv(n,[loi,parametres])` | 10 |

## 3. 微积分

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `diff(f,var,[n]) 导数` | 表达式 f 对 var 的导数（n 阶，默认 1）。例如 diff(sin(x),x)、diff(x^3,x,2)。对 x 求导可用 f'（快捷键 F3）。var 为变量列表时求梯度。 | `diff(f,var,[n])` | sin(x),x |
| `fourier_an(f,x,T,n,a) 余弦系数` | f 的余弦傅里叶系数 | `fourier_an(f,x,T,n,a)` | x^2,x,2*pi,n,-pi |
| `fourier_bn(f,x,T,n,a) 正弦系数` | f 的正弦傅里叶系数 | `fourier_bn(f,x,T,n,a)` | x^2,x,2*pi,n,-pi |
| `fourier_cn(f,x,T,n,a) 复指数系数` | f 的复指数傅里叶系数 | `fourier_cn(f,x,T,n,a)` | x^2,x,2*pi,n,-pi |
| `ilaplace(f,s,x) 逆拉普拉斯` | f 的拉普拉斯逆变换 | `ilaplace(f,s,x)` | s/(s^2+1),s,x |
| `inf 无穷` | 正无穷。负无穷用 -inf，无符号/复无穷用 infinity。快捷键 shift INS | `inf` | oo |
| `integrate(f,x,[a,b]) 积分` | f 对 x 的原函数，如 integrate(x*sin(x),x)。定积分加可选参数 a 和 b，如 integrate(x*sin(x),x,0,pi)。快捷键 SHIFT F3。 | `integrate(f,x,[a,b])` | x*sin(x),x |
| `laplace(f,x,s) 拉普拉斯变换` | f 的拉普拉斯变换 | `laplace(f,x,s)` | sin(x),x,s |
| `limit(f,x=a) 极限` | f 在 x=a 处的极限。加 1 或 -1 表示单侧极限，如 limit(sin(x)/x,x=0) 或 limit(abs(x)/x,x=0,1)。快捷键 SHIFT MIXEDFRAC | `limit(f,x=a)` | sin(x)/x,x=0 |
| `linsolve([eq1,eq2,..],[x,y,..]) 线性方程组` | 解线性方程组。可用 lu 的输出做 O(n^2) 求解（见示例 2）。 | `linsolve([eq1,eq2,..],[x,y,..])` | [x+y=1,x-y=2],[x,y] |
| `revert(p[,x]) 级数反演` | Taylor 级数反演 | `revert(p[,x])` | x+x^2+x^4 |
| `sum(f,k,m,M) 求和` | 求和：f 中 k 从 m 到 M。例：sum(k^2,k,1,n)=>*。快捷键 ALPHA F3 | `sum(f,k,m,M)` | k,k,1,n |
| `tabvar(f,[x=a..b]) 变差表` | 表达式 f 的变差表（可选变量 x 及区间 a..b） | `tabvar(f,[x=a..b])` | sqrt(x^2+x+1) |
| `taylor(f,x=a,n,[polynom]) 泰勒展开` | f 在 x=a 处 n 阶 Taylor 展开（加 polynom 参数可去掉余项）。 | `taylor(f,x=a,n,[polynom])` | sin(x),x=0,5 |

## 4. 算术，加密 crypto

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `% 取模` | a % b 表示 a 模 b | `%` | — |
| `asc(string) ASCII 码` | 字符串的 ASCII 码列表 | `asc(string)` | "Hello" |
| `char(liste) 字符转换` | 将 ASCII 码列表转换为字符串。 | `char(liste)` | [97,98,99] |
| `desolve(equation,t,y) 微分方程解` | 精确求解微分方程。 | `desolve(equation,t,y)` | desolve([y'+y=exp(x),y(0)=1]) |
| `euler(n) 欧拉函数` | 欧拉函数：小于 n 且与 n 互素的整数个数 | `euler(n)` | 25 |
| `gcd(a,b,...) 最大公约数` | 最大公约数。扩展 GCD 参见 iegcd 与 egcd。 | `gcd(a,b,...)` | 23,13 |
| `iabcuv(a,b,c) 整数贝祖` | 求整数 u,v 使 a*u+b*v=c | `iabcuv(a,b,c)` | 23,13,15 |
| `ichinrem([a,m],[b,n]) 中国剩余定理` | 整数中国剩余定理：a mod m 且 b mod n | `ichinrem([a,m],[b,n])` | [3,13],[2,7] |
| `idivis(n) 约数列表` | 返回整数 n 的约数列表。 | `idivis(n)` | 10 |
| `iegcd(a,b) 整数GCD` | 求整数 u,v,d 使 a*u+b*v=d=gcd(a,b) | `iegcd(a,b)` | 23,13 |
| `ifactor(n) 整数分解` | 整数因式分解（不宜太大！）。快捷键 n=>* | `ifactor(n)` | — |
| `iquo(a,b) 整数商` | a 除以 b 的整数商。 | `iquo(a,b)` | 23,13 |
| `irem(a,b) 整数余数` | a 除以 b 的整数余数。 | `irem(a,b)` | 23,13 |
| `isprime(n) 素数判定` | n 为素数返回 1，否则返回 0。 | `isprime(n)` | 11 |
| `lcm(a,b,...) 最小公倍数` | 最小公倍数。 | `lcm(a,b,...)` | 23,13 |
| `powmod(a,n,p) 模幂` | 返回 a^n mod p。 | `powmod(a,n,p)` | 123,456,789 |

## 5. 复数

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `abs(x) 绝对值/范数` | x 的绝对值或范数 | `abs(x)` | -3 |
| `arg(z) 辐角` | 复数 z 的辐角 | `arg(z)` | 1+i |
| `conj(z) 共轭` | z 的共轭复数。 | `conj(z)` | 1+i |
| `evalc(z) 复数展开` | 把 z 写成 x+i*y。 | `evalc(z)` | 1/(1+i*sqrt(3)) |
| `im(z) 虚部` | 虚部。 | `im(z)` | 1+i |
| `re(z) 实部` | 实部。 | `re(z)` | 1+i |

## 6. 曲线

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `cfactor(p) 复数分解` | 在复数域上分解因式。 | `cfactor(p)` | x^4-1 |
| `cpartfrac(p,x) 复部分分式` | p 关于 x 在复数域上的部分分式分解。 | `cpartfrac(p,x)` | 1/(x^4-1) |
| `csolve(equation,x) 复数解方程` | 在复数域精确求解方程（或多项式方程组）。 | `csolve(equation,x)` | x^2+x+1=0 |
| `linetan(expr,x,x0) 切线` | 曲线在 x=x0 处的切线。 | `linetan(expr,x,x0)` | sin(x),x,pi/2 |
| `mult_c_conjugate 复共轭` | 乘以复共轭。 | `mult_c_conjugate` | 1+2*i |
| `plotfield(f(t,y),[t=tmin..tmax,y=ymin..ymax]) 向量场` | 绘制微分方程 y'=f(t,y) 的场（加 plotode=[t0,y0] 可选画一条解曲线） | `plotfield(f(t,y),[t=tmin..tmax,y=ymin..ymax])` | sin(t*y),[t=-3..3,y=-3..3],plotode=[0,1] |
| `plotfunc(expr,[x,y])`* | Xcas: graph of a 3d function | `plotfunc(expr,[x,y])` | x^2-y^2,[x,y] |
| `plotlist(list) 列表绘图` | 绘制一个列表 | `plotlist(list)` | [3/2,2,1,1/2,3,2,3/2] |
| `plotode(f(t,y),[t=tmin..tmax,y],[t0,y0]) 解曲线` | 绘制微分方程 y'=f(t,y)、y(t0)=y0 的解曲线。 | `plotode(f(t,y),[t=tmin..tmax,y],[t0,y0])` | sin(t*y),[t=-3..3,y],[0,1] |
| `plotparam([x,y],t) 参数曲线` | 参数曲线。例：plotparam([sin(3t),cos(2t)],t,0,pi) 或 plotparam(exp(i*t),t,0,pi) | `plotparam([x,y],t)` | [sin(3t),cos(2t)],t,0,pi |
| `plotpolar(r,theta) 极坐标图` | 极坐标曲线。 | `plotpolar(r,theta)` | cos(3*x),x,0,pi |
| `plotseq(f(x),x=[u0,m,M],n) 迭代图` | 在 [m,M] 上绘制 f(x) 与递推 u_{n+1}=f(u_n)（初值 u0）的前 n 项。 | `plotseq(f(x),x=[u0,m,M],n)` | sqrt(2+x),x=[6,0,7],5 |
| `point(x,y[,z]) 点` | 点 | `point(x,y[,z])` | 1,2 |

## 7. 多项式

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `abcuv(a,b,c) 多项式贝祖` | 求多项式 u,v 使 a*u+b*v=c | `abcuv(a,b,c)` | x+1,x^2-2,x |
| `coeff(p,x,n) 系数` | 多项式 p 中 x^n 的系数。 | `coeff(p,x,n)` | — |
| `degree(p,x) 次数` | 多项式 p 关于 x 的次数。 | `degree(p,x)` | x^4-1 |
| `denom(x) 分母` | 表达式 x 的分母。 | `denom(x)` | 3/4 |
| `egcd(A,B) 多项式GCD` | 求多项式 U,V,D 使 A*U+B*V=D=gcd(A,B) | `egcd(A,B)` | x^2+3x+1,x^2-5x-1 |
| `hermite(n) 埃尔米特多项式` | 第 n 个 Hermite 多项式 | `hermite(n)` | 10 |
| `interp(X,Y) 插值` | 在点 (xi,yi) 处 Lagrange 插值（X 为 xi 列表、Y 为 yi 列表）。第三参数给 interp 时返回差商列表。 | `interp(X,Y)` | [1,2,3,4,5],[0,1,3,4,4] |
| `laguerre(n,a,x) 拉盖尔多项式` | 第 n 个 Laguerre 多项式（默认 a=0）。 | `laguerre(n,a,x)` | 10 |
| `lcoeff(p,x) 首项系数` | 多项式 p 关于 x 的首项系数。 | `lcoeff(p,x)` | x^4-1 |
| `legendre(n) 勒让德多项式` | 第 n 个 Legendre 多项式。 | `legendre(n)` | 10 |
| `numer(x) 分子` | x 的分子。 | `numer(x)` | 3/4 |
| `plot(expr,x) 绘图` | 绘制表达式。例：plot(sin(x))、plot(ln(x),x,0,5)、plot(x^2-y^2)、plot(x^2-y^2<1)、plot(x^2-y^2=1) | `plot(expr,x)` | ln(x),x,0,5 |
| `proot(p) 多项式求根` | 返回多项式 p 的实根与复根。例：proot([1,2.1,3,4.2]) 或 proot(x^3+2.1*x^2+3x+4.2) | `proot(p)` | x^3+2.1*x^2+3x+4.2 |
| `quo(p,q,x) 综合除法商` | 多项式 p、q 综合除法（变量 x）的商。 | `quo(p,q,x)` | — |
| `rem(p,q,x) 综合除法余` | 多项式 p、q 综合除法（变量 x）的余数 | `rem(p,q,x)` | — |
| `tchebyshev1(n) 第一类切比雪夫` | 第 1 类 Tchebyshev 多项式：cos(n*x)=T_n(cos(x)) | `tchebyshev1(n)` | 10 |
| `tchebyshev2(n) 第二类切比雪夫` | 第 2 类 Tchebyshev 多项式：sin((n+1)*x)=sin(x)*U_n(cos(x)) | `tchebyshev2(n)` | 10 |

## 8. 概率

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `_cdf`* | Suffix to get a cumulative distribution function. Type F2 for inverse cumulative distribution function _icdf suffix. | `_cdf` | _icdf |
| `binomial(n,p,k) 二项分布` | binomial(n,p,k)：n 次试验中恰好 k 次成功的概率（单次成功概率 p）。binomial_cdf(n,p,k) 为最多 k 次成功的概率。binomial_icdf(n,p,t) 返回使 binomial_cdf(n,p,k)>=t 的最小 k | `binomial(n,p,k)` | 10,.5,4 |
| `comb(n,k) 组合数` | 返回组合数 C(n,k) | `comb(n,k)` | 10,4 |
| `erf(x) 误差函数` | x 的误差函数。 | `erf(x)` | 1.2 |
| `erfc(x) 余误差函数` | x 的余误差函数。 | `erfc(x)` | 1.2 |
| `exponentiald(lambda,x) 指数分布` | 参数 lambda 的指数分布。exponentiald_cdf(lambda,x)：P(指数分布<=x)，如 exponentiald_cdf(2,3)；exponentiald_icdf(lambda,t) 返回使概率为 t 的 x，如 exponentiald_icdf(2,0.95) | `exponentiald(lambda,x)` | 5.1,3.4 |
| `factor(p,[x]) 因式分解` | 对多项式 p 因式分解（整数用 ifactor）。快捷键 p=>* | `factor(p,[x])` | x^4-1 |
| `gcd(a,b,...) 最大公约数` | 最大公约数。扩展 GCD 参见 iegcd 与 egcd。 | `gcd(a,b,...)` | 23,13 |
| `lcm(a,b,...) 最小公倍数` | 最小公倍数。 | `lcm(a,b,...)` | 23,13 |
| `normald([mu,sigma],x) 正态分布` | 正态分布概率密度，默认 mu=0、sigma=1。normald_cdf([mu,sigma],x)：P(正态<=x)，如 normald_cdf(1.96)；normald_icdf([mu,sigma],t) 返回使 P(正态<=x)=t 的 x，如 normald_icdf(0.975) | `normald([mu,sigma],x)` | 1.2 |
| `rand() 随机数` | 0 到 1 之间的随机实数 | `rand()` | — |
| `randint(a,b) 随机整数` | a、b 之间（含）的随机整数。Xcas 单参数时返回 1..n 随机整数。 | `randint(a,b)` | 5,25 |
| `uniformd(a,b,x) 均匀分布` | [a,b] 上的均匀分布，密度 1/(b-a) | `uniformd` | — |

## 9. 程序命令

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `! 阶乘/非` | 逻辑非（前缀）或 n 的阶乘（后缀）。 | `!` | 7! |
| `& 逻辑与` | 逻辑与，或 +（同或） | `&` | 1&2 |
| `:=`* | Set variable value. Shortcut SHIFT F1 | `:=` | a:=3 |
| `< 小于` | 快捷键 SHIFT F2 | `<` | — |
| `=>`* | Store value in variable or conversion (touche ->). For example 5=>a or x^4-1=>* or (x+1)^2=>+ or sin(x)^2=>cos. | `=>` | 5=>a |
| `> 大于` | 快捷键 F2 | `>` | — |
| `\ 反斜杠` | \\ 字符 | `\` | — |
| `_ 下划线` | _ 字符，快捷键 (-) | `_` | — |
| `a and b 逻辑与` | 逻辑与 | `and` | — |
| `a or b 逻辑或` | 逻辑或 | `or` | — |
| `assume(hyp) 假设` | 对变量的假设 | `assume(hyp)` | x>1 |
| `bitxor 按位异或` | 异或 | `bitxor` | bitxor(1,2) |
| `black 黑色` | 显示选项 | `black` | display=black |
| `blue 蓝色` | 显示选项 | `blue` | display=blue |
| `circle(center,radius) 圆` | 圆 | `circle(center,radius)` | 2+i,3 |
| `circumcircle(A,B,C)`* | Circumcircle | `circumcircle(A,B,C)` | -1,2+i,3 |
| `clearscreen()`* | Clear screen. | `clearscreen()` | — |
| `cyan 青色` | 显示选项 | `cyan` | display=cyan |
| `display 显示参数` | 显示选项 | `display` | display=red |
| `draw_arc(x1,y1,rx,ry,theta1,theta2,c) 椭圆弧` | 像素化的椭圆弧。 | `draw_arc(x1,y1,rx,ry,theta1,theta2,c)` | 100,100,60,80,0,pi,magenta |
| `draw_circle(x1,y1,r,c) 像素圆` | 像素化圆。选项：filled（实心） | `draw_circle(x1,y1,r,c)` | 100,100,60,cyan+filled |
| `draw_line(x1,y1,x2,y2,c) 像素直线` | 像素化直线。 | `draw_line(x1,y1,x2,y2,c)` | 100,50,300,200,blue |
| `draw_pixel(x,y,color) 像素` | 给像素 x,y 着色。draw_pixel() 用于同步屏幕。 | `draw_pixel(x,y,color)` | — |
| `draw_polygon([[x1,y1],...],c) 像素多边形` | 像素化多边形。 | `draw_polygon([[x1,y1],...],c)` | [[100,50],[30,20],[60,70]],red+filled |
| `draw_rectangle(x,y,w,h,c) 矩形` | 矩形。 | `draw_rectangle(x,y,w,h,c)` | 100,50,30,20,red+filled |
| `draw_string(s,x,y,c) 显示字符串` | 在像素 x,y 处显示字符串 s | `draw_string(s,x,y,c)` | "Bonjour",80,60 |
| `eval(f) 执行` | 执行 f。 | `eval(f)` | — |
| `filled 填充` | 显示选项 | `filled` | — |
| `gl_x X 范围` | 绘图设置 X：gl_x=xmin..xmax | `gl_x` | gl_x=0..2 |
| `gl_y Y 范围` | 绘图设置 Y：gl_y=ymin..ymax | `gl_y` | gl_y=-1..1 |
| `green 绿色` | 显示选项 | `green` | display=green |
| `incircle(A,B,C)`* | Incircle | `incircle(A,B,C)` | -1,2+i,3 |
| `line(equation) 直线` | 直线方程 | `line(equation)` | y=2x+1 |
| `line_width_ 线宽` | 线宽前缀（2 到 8） | `line_width_` | — |
| `magenta 品红` | 显示选项 | `magenta` | display=magenta |
| `not(x) 逻辑非` | 逻辑非。 | `not(x)` | — |
| `plus_point 点样式` | 显示选项 | `plus_point` | display=blue+plus_point |
| `polygon(list) 多边形` | 由顶点列表给出的闭合多边形。 | `polygon(list)` | 1-i,2+i,3,3-2i |
| `purge(x) 清除变量` | 清除变量 x 的赋值。快捷键 SHIFT-FORMAT | `purge(x)` | — |
| `python(f) Python语法` | 用 Python 语法显示 f。 | `python(f)` | — |
| `read("filename") 读文件` | 读取文件。 | `read("` | — |
| `red 红色` | 显示选项 | `red` | display=red |
| `rgb(r,g,b) RGB 颜色` | 由红绿蓝 0-255 定义颜色 | `rgb(r,g,b)` | 255,0,255 |
| `rhombus_point 菱形点` | 显示选项 | `rhombus_point` | display=magenta+rhombus_point |
| `segment(A,B)`* | 线段 | `segment(A,B)` | 1,2+i |
| `seq(expr,var,a,b) 序列生成` | 由表达式生成列表。 | `seq(expr,var,a,b)` | j^2,j,1,10 |
| `square_point 方点` | 显示选项 | `square_point` | display=cyan+square_point |
| `star_point 星点` | 显示选项 | `star_point` | display=magenta+star_point |
| `triangle_point 三角形点` | 显示选项 | `triangle_point` | display=yellow+triangle_point |
| `write("filename",var) 写文件` | 把一个或多个变量保存到文件。例：f(x):=x^2; write("func_f",f)。 | `write("` | — |
| `yellow 黄色` | 显示选项 | `yellow` | display=yellow |
| `\| 逻辑或` | 逻辑或 | `\|` | 1\|2 |
| `~ 按位取反` | 补集/按位取反 | `~` | ~7 |

## 10. 实数

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `% 取模` | a % b 表示 a 模 b | `%` | — |
| `approx(x) 近似值` | x 的近似值。快捷键 S-D | `approx(x)` | pi |
| `axes`* | Axes visible or not axes=1 or 0 | `axes` | axes=0 |
| `ceil(x) 向上取整` | 不小于 x 的最小整数 | `ceil(x)` | 1.2 |
| `exact(x) 精确化` | 把 x 转换为有理数。快捷键 shift S-D | `exact(x)` | 1.2 |
| `float(x) 浮点数` | 把 x 转换为浮点值。 | `float(x)` | pi |
| `floor(x) 向下取整` | 不大于 x 的最大整数 | `floor(x)` | pi |
| `sign(x)`* | Returns -1 if x is negative, 0 if x is zero and 1 if x is positive. | `sign(x)` | — |

## 11. 求解

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `abs(x) 绝对值/范数` | x 的绝对值或范数 | `abs(x)` | -3 |
| `csolve(equation,x) 复数解方程` | 在复数域精确求解方程（或多项式方程组）。 | `csolve(equation,x)` | x^2+x+1=0 |
| `desolve(equation,t,y) 微分方程解` | 精确求解微分方程。 | `desolve(equation,t,y)` | desolve([y'+y=exp(x),y(0)=1]) |
| `fsolve(equation,x=a..b) 数值解方程` | 在区间 a..b 内近似求解方程。 | `fsolve(equation,x=a..b)` | cos(x)=x,x=0..1 |
| `linsolve([eq1,eq2,..],[x,y,..]) 线性方程组` | 解线性方程组。可用 lu 的输出做 O(n^2) 求解（见示例 2）。 | `linsolve([eq1,eq2,..],[x,y,..])` | [x+y=1,x-y=2],[x,y] |
| `odesolve(f(t,y),[t,y],[t0,y0],t1) 微分方程数值解` | 微分方程 y'=f(t,y)、y(t0)=y0 的近似解在 t=t1 的值（加 curve 参数返回 y 的中间值） | `odesolve(f(t,y),[t,y],[t0,y0],t1)` | sin(t*y),[t,y],[0,1],2 |
| `rsolve(equation,u(n),[init]) 递推解` | 解递推关系。 | `rsolve(equation,u(n),[init])` | u(n+1)=2*u(n)+3,u(n),u(0)=1 |
| `solve(equation,x) 求解` | 对 x 精确求解方程（或多项式方程组）。复数解用 csolve，线性方程组用 linsolve。快捷键 SHIFT XthetaT | `solve(equation,x)` | x^2-x-1=0,x |

## 12. 统计

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `_plot 回归曲线名` | 回归曲线名称后缀。 | `_plot` | X,Y:=[1,2,3,4,5],[0,1,3,4,4];polynomial_regression_plot(X,Y,2);scatterplot(X,Y) |
| `barplot(list) 条形图` | list 中一维统计数据的条形图 | `barplot(list)` | [3/2,2,1,1/2,3,2,3/2] |
| `camembert(list) 饼图` | list 中一维统计序列的饼图。 | `camembert(list)` | [["France",6],["Germany",12],["Switzerland",5]] |
| `correlation(l1,l2) 相关系数` | 列表 l1 与 l2 的相关系数 | `correlation(l1,l2)` | [1,2,3,4,5],[0,1,3,4,4] |
| `covariance(l1,l2) 协方差` | 列表 l1 与 l2 的协方差 | `covariance(l1,l2)` | [1,2,3,4,5],[0,1,3,4,4] |
| `exponential_regression(Xlist,Ylist) 指数回归` | 指数回归。 | `exponential_regression(Xlist,Ylist)` | [1,2,3,4,5],[0,1,3,4,4] |
| `exponential_regression_plot(Xlist,Ylist) 指数回归图` | 指数回归曲线图。 | `exponential_regression_plot(Xlist,Ylist)` | X,Y:=[1,2,3,4,5],[0,1,3,4,4];exponential_regression_plot(X,Y);scatterplot(X,Y) |
| `histogram(list,min,size) 直方图` | list 数据的直方图，分组从 min 开始、组宽 size。 | `histogram(list,min,size)` | ranv(100,uniformd,0,1),0,0.1 |
| `linear_regression(Xlist,Ylist) 线性回归` | 线性回归。 | `linear_regression(Xlist,Ylist)` | [1,2,3,4,5],[0,1,3,4,4] |
| `linear_regression_plot(Xlist,Ylist) 线性回归图` | 线性回归曲线图。 | `linear_regression_plot(Xlist,Ylist)` | X,Y:=[1,2,3,4,5],[0,1,3,4,4];linear_regression_plot(X,Y);scatterplot(X,Y) |
| `logarithmic_regression(Xlist,Ylist) 对数回归` | 对数回归。 | `logarithmic_regression(Xlist,Ylist)` | [1,2,3,4,5],[0,1,3,4,4] |
| `logarithmic_regression_plot(Xlist,Ylist) 对数回归图` | 对数回归曲线图。 | `logarithmic_regression_plot(Xlist,Ylist)` | X,Y:=[1,2,3,4,5],[0,1,3,4,4];logarithmic_regression_plot(X,Y);scatterplot(X,Y) |
| `mean(l) 平均值` | 列表 l 的算术平均值 | `mean(l)` | [3/2,2,1,1/2,3,2,3/2] |
| `median(l) 中位数` | 中位数 | `median(l)` | [3/2,2,1,1/2,3,2,3/2] |
| `polygonscatterplot(Xlist,Ylist) 点线图` | 绘制点与折线。 | `polygonscatterplot(Xlist,Ylist)` | [1,2,3,4,5],[0,1,3,4,4] |
| `polynomial_regression(Xlist,Ylist,n) 多项式回归` | 多项式回归，次数 <= n。 | `polynomial_regression(Xlist,Ylist,n)` | [1,2,3,4,5],[0,1,3,4,4],2 |
| `polynomial_regression_plot(Xlist,Ylist,n) 多项式回归图` | 多项式回归曲线，次数 <= n。 | `polynomial_regression_plot(Xlist,Ylist,n)` | X,Y:=[1,2,3,4,5],[0,1,3,4,4];polynomial_regression_plot(X,Y,2);scatterplot(X,Y) |
| `power_regression(Xlist,Ylist,n) 幂回归` | 幂回归。 | `power_regression(Xlist,Ylist,n)` | [1,2,3,4,5],[0,1,3,4,4] |
| `power_regression_plot(Xlist,Ylist,n) 幂回归图` | 幂回归曲线图 | `power_regression_plot(Xlist,Ylist,n)` | X,Y:=[1,2,3,4,5],[0,1,3,4,4];power_regression_plot(X,Y);scatterplot(X,Y) |
| `quartile1(l) 第一四分位` | 第 1 四分位数 | `quartile1(l)` | [3/2,2,1,1/2,3,2,3/2] |
| `quartile3(l) 第三四分位` | 第 3 四分位数 | `quartile3(l)` | [3/2,2,1,1/2,3,2,3/2] |
| `scatterplot(Xlist,Ylist) 散点图` | 绘制点 | `scatterplot(Xlist,Ylist)` | [1,2,3,4,5],[0,1,3,4,4] |
| `stddev(l) 标准差` | 列表 l 的标准差 | `stddev(l)` | [3/2,2,1,1/2,3,2,3/2] |

## 13. 三角函数

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `exp2trig(expr) 指数转三角` | 把复指数转换为 sin/cos | `exp2trig(expr)` | exp(i*x) |
| `halftan(expr) 半角正切` | 用 tan(angle/2) 表示 cos、sin、tan。 | `halftan(expr)` | cos(x) |
| `tcollect(expr) 三角合并` | 三角函数的线性化与合并。 | `tcollect(expr)` | sin(x)+cos(x) |
| `texpand(expr) 三角展开` | 展开三角、指数与对数函数。 | `texpand(expr)` | sin(3x) |
| `tlin(expr) 三角线性化` | 三角函数的线性化。 | `tlin(expr)` | sin(x)^3 |
| `trig2exp(expr) 三角转指数` | 把复指数转换为三角函数 | `trig2exp(expr)` | cos(x)^3 |
| `trigcos(expr) 转为余弦` | 把 sin^2、tan^2 转换为 cos^2。 | `trigcos(expr)` | sin(x)^4 |
| `trigsin(expr) 转为正弦` | 把 cos^2、tan^2 转换为 sin^2。 | `trigsin(expr)` | cos(x)^4 |
| `trigtan(expr) 转为正切` | 把 cos^2、sin^2 转换为 tan^2。 | `trigtan(expr)` | cos(x)^4 |

## 14. 列表

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `edit list  列表向导` | 列表创建向导。 | `list` | — |
| `append 追加` | 在列表末尾添加一个元素 | `append` | l.append(x) |
| `extend 合并列表` | 合并两个列表。注意 + 不合并列表（向量才相加） | `extend` | l1.extend(l2) |
| `map(f,l) 映射` | 把函数 f 映射到列表 l 的元素上。 | `map(f,l)` | lambda x:x*x,[1,2,3] |
| `sorted(l) 排序` | 列表排序。 | `sorted(l)` | [3/2,2,1,1/2,3,2,3/2] |

## 15. 矩阵

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `edit matrix  矩阵向导` | 矩阵创建向导。 | `matrix` | — |
| `charpoly(M,x) 特征多项式` | 矩阵 M 关于变量 x 的特征多项式。 | `charpoly(M,x)` | [[1,2],[3,4]],x |
| `cond(A,[1,2,inf]) 条件数` | 矩阵关于给定范数的条件数（默认范数 1） | `cond(A,[1,2,inf])` | [[1,2],[3,4]] |
| `det(A) 行列式` | 矩阵 A 的行列式。 | `det(A)` | [[1,2],[3,4]] |
| `eigenvals(A)`* | Eigenvalues of matrix A. | `eigenvals(A)` | [[1,2],[3,4]] |
| `eigenvects(A) 特征向量` | 矩阵 A 的特征向量。 | `eigenvects(A)` | [[1,2],[3,4]] |
| `hilbert(n) 希尔伯特矩阵` | n 阶 Hilbert 矩阵。 | `hilbert(n)` | 4 |
| `idn(n) 单位矩阵` | n 阶单位矩阵 | `idn(n)` | 4 |
| `inv(A) 逆矩阵` | A 的逆矩阵。 | `inv(A)` | [[1,2],[3,4]] |
| `jordan(A) 若尔当标准形` | 矩阵 A 的 Jordan 标准形：返回 P 和 D 使 P^-1*A*P=D | `jordan(A)` | [[1,2],[3,4]] |
| `lu(A) LU 分解` | 矩阵 A 的 LU 分解：P*A=L*U | `lu(A)` | [[1,2],[3,4]] |
| `matpow(A,n) 矩阵幂` | 返回矩阵 A^n | `matpow(A,n)` | [[1,2],[3,4]],n |
| `matrix(r,c,func) 矩阵生成` | 由定义函数生成矩阵。 | `matrix(r,c,func)` | 2,3,(j,k)->j^k |
| `qr(A) QR 分解` | A=Q*R 分解：Q 正交、R 上三角 | `qr(A)` | [[1,2],[3,4]] |
| `ranm(n,m,[loi,parametres]) 随机矩阵` | 整数系数或按概率律的随机矩阵（向量用 ranv）。例：ranm(2,3)、ranm(3,2,binomial,20,.3)、ranm(4,2,normald,0,1) | `ranm(n,m,[loi,parametres])` | 3,3 |
| `svd(A) SVD 分解` | 奇异值分解：返回正交 U、奇异值向量 S、正交 Q，使 A=U*diag(S)*tran(Q) | `svd(A)` | [[1,2],[3,4]] |
| `trace(A) 迹` | 矩阵 A 的迹。 | `trace(A)` | [[1,2],[3,4]] |
| `transpose(A) 转置` | 矩阵 A 的转置。共轭转置命令为 trn(A) 或 A^*。 | `transpose(A)` | [[1,2],[3,4]] |

## 16. 程序

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `loop for 循环` | 定义循环。 | `for` | for |
| `loop in list 列表迭代` | 对列表的所有元素循环。 | `for in` | for in |
| `loop while 当型循环` | 不定次循环。 | `while` | while |
| `test if 条件` | 条件测试。 | `if` | if |
| `test else 否则` | 条件为假时执行 | `else` | — |
| `function def 函数定义` | 函数定义。 | `f(x):=` | f(x):= |
| `local j,k; 局部变量` | 局部变量声明（Xcas） | `local` | — |
| `range(a,b) 区间` | 区间 [a,b)（含 a 不含 b） | `range(a,b)` | in range(1,10) |
| `return res 返回` | 退出当前函数并返回 res。 | `return` | — |
| `# 注释` | Python 注释；Xcas 注释用 //。快捷键 ALPHA F2 | `#` | — |
| `debug(f(args)) 调试` | 单步模式运行用户函数 f。 | `debug(f(args))` | — |
| `elif test 否则如果` | 级联条件测试 | `elif` | — |
| `from math/... import * 导入模块` | 访问 math 或随机函数（[random]）、以及英文命令名的海龟模块 [turtle]。KhiCAS 中无需 import math | `from math import *` | from random import * |
| `input() 键盘输入` | 从键盘读入字符串 | `input()` | — |
| `print(expr) 打印` | 在控制台输出 expr | `print(expr)` | — |
| `python_compat(0\|1\|2) Python兼容` | python_compat(0) Xcas 语法；python_compat(1) Python 语法（^ 为乘方）；python_compat(2) ^ 为按位异或 | `python_compat(0\|1\|2)` | 0 |
| `time(cmd) 计时` | 运行命令并计时（或设置时钟） | `time(cmd)` | int(1/(x^4+1),x) |

## 17. 修改变量

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `linsolve([eq1,eq2,..],[x,y,..]) 线性方程组` | 解线性方程组。可用 lu 的输出做 O(n^2) 求解（见示例 2）。 | `linsolve([eq1,eq2,..],[x,y,..])` | [x+y=1,x-y=2],[x,y] |

## 18. 物理常数

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `mksa(x)`* | Conversion to MKSA units | `mksa(x)` | — |
| `ufactor(a,b)`* | Factorize unit b in a | `ufactor(a,b)` | 100_J,1_kW |
| `usimplify(a)`* | Simplify unit | `usimplify(a)` | 100_l/10_cm^2 |
| `:=`* | Set variable value. Shortcut SHIFT F1 | `:=` | a:=3 |
| `_F_`* | Faraday constant | `_F_` | — |
| `_G_`* | Gravitation force=_G_*m1*m2/r^2 | `_G_` | — |
| `_NA_`* | Avogadro constant | `_NA_` | — |
| `_PSun_`* | Sun power | `_PSun_` | — |
| `_REarth_`* | Earth radius | `_REarth_` | — |
| `_RSun_`* | Sun radius | `_RSun_` | — |
| `_R_`* | Boltzmann constant (per mol) | `_R_` | — |
| `_StdP_`* | Standard pressure | `_StdP_` | — |
| `_StdT_`* | Standard temperature (0 degre Celsius in Kelvins) | `_StdT_` | — |
| `_Vm_`* | Volume molaire | `_Vm_` | — |
| `_alpha_`* | fine structure constant | `_alpha_` | — |
| `_c_`* | speed of light | `_c_` | — |
| `_epsilon0_`* | vacuum permittivity | `_epsilon0_` | — |
| `_g_`* | Earth gravity (ground) | `_g_` | — |
| `_h_`* | Planck constant | `_h_` | — |
| `_hbar_`* | Planck constant/(2*pi) | `_hbar_` | — |
| `_k_`* | Boltzmann constant | `_k_` | — |
| `_mEarth_`* | Earth mass | `_mEarth_` | — |
| `_me_`* | electron mass | `_me_` | — |
| `_mp_`* | proton mass | `_mp_` | — |
| `_mpme_`* | proton/electron mass-ratio | `_mpme_` | — |
| `_mu0_`* | — | `_mu0_` | — |
| `_phi_`* | magnetic flux quantum | `_phi_` | — |
| `_qe_`* | electron charge | `_qe_` | — |
| `_qme_`* | _q_/_me_ | `_qme_` | — |
| `_sd_`* | Sideral day | `_sd_` | — |
| `_syr_`* | Siderale year | `_syr_` | — |
| `assume(hyp) 假设` | 对变量的假设 | `assume(hyp)` | x>1 |
| `purge(x) 清除变量` | 清除变量 x 的赋值。快捷键 SHIFT-FORMAT | `purge(x)` | — |

## 19. 物理单位

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `=>`* | Store value in variable or conversion (touche ->). For example 5=>a or x^4-1=>* or (x+1)^2=>+ or sin(x)^2=>cos. | `=>` | 5=>a |
| `_(km/h)`* | Speed kilometer per hour | `_(km/h)` | — |
| `_(m/s)`* | Speed meter/second | `_(m/s)` | — |
| `_(m/s^2)`* | Acceleration | `_(m/s^2)` | — |
| `_(m^2/s)`* | Viscosity | `_(m^2/s)` | — |
| `_A`* | Ampere | `_A` | — |
| `_Bq`* | Becquerel | `_Bq` | — |
| `_C`* | Coulomb | `_C` | — |
| `_Ci`* | Curie | `_Ci` | — |
| `_F`* | Farad | `_F` | — |
| `_H`* | Henry | `_H` | — |
| `_Hz`* | Hertz | `_Hz` | — |
| `_J`* | Joule=kg*m^2/s^2 | `_J` | — |
| `_K`* | Temperature in Kelvin | `_K` | — |
| `_Kcal`* | Energy kilo-calorie | `_Kcal` | — |
| `_MeV`* | Energy mega-electron-Volt | `_MeV` | — |
| `_N`* | Force Newton=kg*m/s^2 | `_N` | — |
| `_Ohm`* | Ohm | `_Ohm` | — |
| `_Pa`* | Pressure in Pascal=kg/m/s^2 | `_Pa` | — |
| `_S`* | — | `_S` | — |
| `_Sv`* | Sievert | `_Sv` | — |
| `_T`* | Tesla | `_T` | — |
| `_V`* | Volt | `_V` | — |
| `_W`* | Watt=kg*m^2/s^3 | `_W` | — |
| `_Wb`* | Weber | `_Wb` | — |
| `_cd`* | candela | `_cd` | — |
| `_d`* | day | `_d` | — |
| `_deg`* | degree | `_deg` | — |
| `_eV`* | electron-Volt | `_eV` | — |
| `_ft`* | feet | `_ft` | — |
| `_grad`* | grades (angle unit( | `_grad` | — |
| `_h`* | Hour | `_h` | — |
| `_ha`* | hectare | `_ha` | — |
| `_inch`* | inches | `_inch` | — |
| `_kWh`* | kWh | `_kWh` | — |
| `_kg`* | kilogram | `_kg` | — |
| `_l`* | liter | `_l` | — |
| `_m`* | meter | `_m` | — |
| `_m^2`* | Area in m^2 | `_m^2` | — |
| `_m^3`* | Volume in m^3 | `_m^3` | — |
| `_miUS`* | US miles | `_miUS` | — |
| `_mn`* | minute | `_mn` | — |
| `_rad`* | radians | `_rad` | — |
| `_rem`* | rem | `_rem` | — |
| `_s`* | second | `_s` | — |
| `_tr`* | tour (angle unit) | `_tr` | — |
| `_yd`* | yards | `_yd` | — |

## 20. 几何

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `mksa(x)`* | Conversion to MKSA units | `mksa(x)` | — |
| `ufactor(a,b)`* | Factorize unit b in a | `ufactor(a,b)` | 100_J,1_kW |
| `usimplify(a)`* | Simplify unit | `usimplify(a)` | 100_l/10_cm^2 |
| `altitude(A,B,C) 高线` | 三角形 ABC 中由 A 引出的高 | `altitude(A,B,C)` | 1,i,2+i |
| `area(objet) 面积` | 代数面积 | `area(objet)` | circle(0,1) |
| `barycenter([pnt,coeff],...) 重心` | 一组 [点,系数] 的重心；若系数全相等则执行等重心 | `barycenter([pnt,coeff],...)` | [1,1],[i,1],[2,3] |
| `bisector(A,B,C) 角平分线` | 角 AB、AC 的角平分线 | `bisector(A,B,C)` | 1,i,2+i |
| `center(objet) 中心` | 圆或球心。椭圆/双曲线返回中心、一个焦点和曲线上一点；抛物线返回焦点和顶点。 | `center(objet)` | circle(0,1) |
| `conic(expression) 圆锥曲线` | 由二次多项式方程或 5 个点给出的圆锥曲线 | `conic(expression)` | x^2+x*y+y^2=5 |
| `coordinates(object) 坐标` | （笛卡尔）坐标 | `coordinates(object)` | point(1,2) |
| `ellipse(F1,F2,M) 椭圆` | 由两个焦点和一点确定的椭圆 | `ellipse(F1,F2,M)` | -1,1,2 |
| `equation(object) 方程` | 笛卡尔方程。参数方程用 parameq。 | `equation(object)` | circle(0,1) |
| `homothety(center,ratio,object) 位似` | 对象按比例 ratio 的中心对称（位似）像 | `homothety(center,ratio,object)` | 0,2,circle(1,1) |
| `hyperbola(F1,F2,M) 双曲线` | 由两个焦点和一点给出的双曲线 | `hyperbola(F1,F2,M)` | -2-i,2+i,1 |
| `is_collinear(A,B,C)`* | Returns 1 if A, B, C are collinear, 0 otherwise | `is_collinear(A,B,C)` | 1,i,-1 |
| `is_concyclic(A,B,C,D)`* | Returns 1 if A, B, C, D are concyclic, 0 otherwise | `is_concyclic(A,B,C,D)` | 1,i,-1,-i |
| `is_element(A,G)`* | Returns 1 if A belongs to G, 0 otherwise. | `is_element(A,G)` | point(0),circle(0,1) |
| `is_parallel(D,E)`* | Returns 1 if D and E are parallel, 0 otherwise | `is_parallel(D,E)` | line(y=x),line(y=-x) |
| `is_perpendicular(D,E)`* | Returns 1 if D and E are perpendicular, 0 otherwise | `is_perpendicular(D,E)` | line(y=x),line(y=-x) |
| `median_line(A,B,C) 中线` | 三角形 ABC 中由顶点 A 引出的中线 | `median_line(A,B,C)` | 1,i,2+i |
| `midpoint(A,B) 中点` | 线段 AB 的中点 | `midpoint(A,B)` | 1,i |
| `parabola(F,A) 抛物线` | 由焦点和顶点给出的抛物线 | `parabola(F,A)` | -2-i,2+i |
| `parameq(object) 参数方程` | 参数方程。笛卡尔方程用 equation | `parameq(object)` | circle(0,1) |
| `perpen_bisector(A,B) 垂直平分线` | 线段 AB 的垂直平分线 | `perpen_bisector(A,B)` | 1,i |
| `projection(obj1,obj2) 投影` | obj2 在 obj1 上的投影 | `projection(obj1,obj2)` | line(y=x),point(2,3) |
| `radius(objet) 半径` | 圆或球的半径 | `radius(objet)` | circle(0,1) |
| `reflection(obj1,obj2) 反射` | obj2 关于 obj1 的对称/反射 | `reflection(obj1,obj2)` | line(y=x),cercle(1,1) |
| `rotation(center,angle,objcet) 旋转` | 对象绕一点旋转的像 | `rotation(center,angle,objcet)` | 2-i,pi/2,circle(0,1) |
| `similarity(center,ratio,angle,object) 相似变换` | 对象的相似（位似+旋转）像 | `similarity(center,ratio,angle,object)` | 0,2,pi/2,circle(1,1) |
| `translation(vect,obj) 平移` | 对象按向量 vect 平移 | `translation(vect,obj)` | [1,2],cercle(0,1) |
| `triangle(A,B,C) 三角形` | 由 3 个顶点给出的三角形 | `triangle(A,B,C)` | 1+i,1-i,-1 |
| `vertices(objet) 顶点` | 多边形或多面体的顶点列表 | `vertices(objet)` | triangle(1,i,2) |

## 21. 三维

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `=>`* | Store value in variable or conversion (touche ->). For example 5=>a or x^4-1=>* or (x+1)^2=>+ or sin(x)^2=>cos. | `=>` | 5=>a |
| `circle(center,radius) 圆` | 圆 | `circle(center,radius)` | 2+i,3 |
| `circumcircle(A,B,C)`* | Circumcircle | `circumcircle(A,B,C)` | -1,2+i,3 |
| `cone(A,v,theta,[h]) 圆锥` | 锥体：顶点 A、方向 v、半角 theta（[可带高 h 和 -h]） | `cone(A,v,theta,[h])` | [0,0,0],[0,0,1],pi/6 |
| `cube(A,B,C) 立方体` | 棱为 AB、一个面在平面 ABC 内的立方体 | `cube(A,B,C)` | [0,0,0],[1,0,0],[0,1,0] |
| `cylinder(A,v,r,[h]) 圆柱` | 轴 A,v、半径 r 的圆柱（[可带高 h]） | `cylinder(A,v,r,[h])` | [0,0,0],[0,1,0],2 |
| `dodecahedron(A,B,C) 正十二面体` | 棱 AB、一个面在平面 ABC 内的正十二面体 | `dodecahedron(A,B,C)` | [0,0,0],[0,2,sqrt(5)/2+3/2],[0,0,1] |
| `icosahedron(A,B,C) 正二十面体` | 中心 A、顶点 B 的正二十面体（平面 ABC 含距 B 最近 5 顶点之一） | `icosahedron(A,B,C)` | [0,0,0],[sqrt(5),0,0],[1,2,0] |
| `incircle(A,B,C)`* | Incircle | `incircle(A,B,C)` | -1,2+i,3 |
| `inter(A,B)`* | Intersections list. Run single_inter if intersection is unique. | `inter(A,B)` | line(y=x),circle(0,1) |
| `line(equation) 直线` | 直线方程 | `line(equation)` | y=2x+1 |
| `octahedron(A,B,C) 正八面体` | 棱 AB、一个面在平面 ABC 内的正八面体 | `octahedron(A,B,C)` | [0,0,0],[3,0,0],[0,1,0] |
| `plane(equation)`* | Plane given by equation or by 3 points | `plane(equation)` | z=x+y-1 |
| `plot(expr,x) 绘图` | 绘制表达式。例：plot(sin(x))、plot(ln(x),x,0,5)、plot(x^2-y^2)、plot(x^2-y^2<1)、plot(x^2-y^2=1) | `plot(expr,x)` | ln(x),x,0,5 |
| `point(x,y[,z]) 点` | 点 | `point(x,y[,z])` | 1,2 |
| `polygon(list) 多边形` | 由顶点列表给出的闭合多边形。 | `polygon(list)` | 1-i,2+i,3,3-2i |
| `polyhedron(A,B,C,D,...) 多面体` | 顶点 A,B,C,D,... 的凸多面体。 | `polyhedron(A,B,C,D,...)` | [0,0,0],[0,5,0],[0,0,5],[1,2,6] |
| `quadric(equation) 二次曲面` | 由方程（或 9 个点）给出的二次曲面 | `quadric(equation)` | x^2-y^2+z^2 |
| `segment(A,B)`* | 线段 | `segment(A,B)` | 1,2+i |
| `single_inter(A,B)`* | First intersection. Run inter for a list of intersections. | `single_inter(A,B)` | line(y=x),line(x+y=3) |
| `sphere(A,r) 球` | 中心 A、半径 r 的球（或直径 AB 的球） | `sphere(A,r)` | [0,0,0],1 |

## 22. 海龟

| 命令 | 用途 | 用法 / 参数 | 示例 |
|------|------|------------|------|
| `avance n 前进` | 海龟前进 n 步，默认 n=10 | `avance` | avance 30 |
| `baisse_crayon  落笔` | 海龟移动时落笔（画线） | `baisse_crayon` | — |
| `barycenter([pnt,coeff],...) 重心` | 一组 [点,系数] 的重心；若系数全相等则执行等重心 | `barycenter([pnt,coeff],...)` | [1,1],[i,1],[2,3] |
| `cache_tortue  隐藏海龟` | 隐藏海龟（图形画完后）。 | `cache_tortue` | — |
| `center(objet) 中心` | 圆或球心。椭圆/双曲线返回中心、一个焦点和曲线上一点；抛物线返回焦点和顶点。 | `center(objet)` | circle(0,1) |
| `coordinates(object) 坐标` | （笛卡尔）坐标 | `coordinates(object)` | point(1,2) |
| `crayon  画笔颜色` | 海龟画线的颜色 | `crayon` | crayon red |
| `disque n 实心圆` | 与海龟相切的实心圆，半径 n。disque n,theta 画 theta 度的实心扇形；disque n,theta,segment 画圆弧段。 | `disque` | disque 30 |
| `equation(object) 方程` | 笛卡尔方程。参数方程用 parameq。 | `equation(object)` | circle(0,1) |
| `line(equation) 直线` | 直线方程 | `line(equation)` | y=2x+1 |
| `midpoint(A,B) 中点` | 线段 AB 的中点 | `midpoint(A,B)` | 1,i |
| `montre_tortue  显示海龟` | 显示海龟 | `montre_tortue` | — |
| `parameq(object) 参数方程` | 参数方程。笛卡尔方程用 equation | `parameq(object)` | circle(0,1) |
| `pas_de_cote n 横向跳步` | 海龟横向跳 n 步，默认 n=10 | `pas_de_cote` | pas_de_cote 30 |
| `plotfunc(expr,[x,y])`* | Xcas: graph of a 3d function | `plotfunc(expr,[x,y])` | x^2-y^2,[x,y] |
| `radius(objet) 半径` | 圆或球的半径 | `radius(objet)` | circle(0,1) |
| `rectangle_plein a,b 实心矩形` | 从海龟位置直接画实心矩形；省略 b 时 b==a | `rectangle_plein` | rectangle_plein 30 |
| `recule n 后退` | 海龟后退 n 步，默认 n=10 | `recule` | recule 30 |
| `rond n 圆` | 与海龟相切的圆，半径 n。rond n,theta 画 theta 度的圆弧 | `rond` | — |
| `rotation(center,angle,objcet) 旋转` | 对象绕一点旋转的像 | `rotation(center,angle,objcet)` | 2-i,pi/2,circle(0,1) |
| `saute n 跳跃` | 海龟跳 n 步，默认 n=10 | `saute` | saute 30 |
| `tourne_droite n 右转` | 海龟右转 n 度，默认 n=90 | `tourne_droite` | — |
| `tourne_gauche n 左转` | 海龟左转 n 度，默认 n=90 | `tourne_gauche` | — |
| `vertices(objet) 顶点` | 多边形或多面体的顶点列表 | `vertices(objet)` | triangle(1,i,2) |

