# tmux -- tmux において East Asian Ambiguous Character を全角文字の幅で表示する為の修正版

## 概要

[tmux][TMUX] において、 Unicode の規格における東アジア圏の各種文字のうち、いわゆる "◎" や "★" 等の記号文字及び罫線文字等、 [East_Asian_Width 特性の値が A (Ambiguous) となる文字][EAWA] (以下、 [East Asian Ambiguous Character][EAWA]) が、日本語環境で文字幅を適切に扱うことが出来ずに表示が乱れる問題が発生しています。

この git リポジトリに置かれている [tmux][TMUX] のソースコードは、 [tmux][TMUX] において [East Asian Ambiguous Character][EAWA] の幅を漢字や全角カナ文字等と同じ幅 2 で表示するように修正したものです。

また、この [tmux][TMUX] のソースコードには、 [koie 氏][KOIE]によって作成された [tmux][TMUX] の[画面分割におけるボーダーラインの罫線文字を判別し、適切に描画するためのソースコードの修正][PANE]が含まれています。

なお、この git リポジトリに置かれている [tmux][TMUX] のソースコードは、 [git リポジトリ "tmux 2.5 以降において East Asian Ambiguous Character を全角文字の幅で表示する"][DIFF] に置かれた差分ファイルの作成用のリポジトリです。

実際に [East Asian Ambiguous Character][EAWA] に対応した [tmux][TMUX] をビルドする際には、[前述の git リポジトリ][DIFF]を参照して、オリジナルの [tmux][TMUX] のソースコードに差分ファイルを適用してビルドするようにして下さい。

## 使用条件

この git リポジトリに置かれている [tmux][TMUX] のソースコードは、 [Nicholas Marriott 氏][NICM]を始めとする [tmux の開発コミュニティ][TMUX] によるコードを、 [koie 氏][KOIE]及び [Z.OOL. (mailto:zool@zool.jpn.org)][ZOOL] によって、 [East Asian Ambiguous Character][EAWA] の幅を漢字や全角カナ文字等と同じ幅 2 で表示し、また、 [tmux][TMUX] の画面分割におけるボーダーラインの罫線文字を判別し適切に描画するように修正したものです。

従って、本ソースコードは、 [tmux][TMUX] のライセンスと同様である [ISC License][ISCL] に基づいて配布されるものとします。詳細については、本リポジトリに同梱する ```LICENSE``` を参照して下さい。

## 追記

以下に、オリジナルの [tmux][TMUX] のソースコードの [README.md][READ] の原文を示します。

----
# Welcome to tmux!

tmux is a terminal multiplexer: it enables a number of terminals to be created,
accessed, and controlled from a single screen. tmux may be detached from a
screen and continue running in the background, then later reattached.

This release runs on OpenBSD, FreeBSD, NetBSD, Linux, OS X and Solaris.

## Dependencies

tmux depends on [libevent](https://libevent.org) 2.x, available from [this
page](https://github.com/libevent/libevent/releases/latest).

It also depends on [ncurses](https://www.gnu.org/software/ncurses/), available
from [this page](https://invisible-mirror.net/archives/ncurses/).

## Installation

### From release tarball

To build and install tmux from a release tarball, use:

~~~bash
./configure && make
sudo make install
~~~

tmux can use the utempter library to update utmp(5), if it is installed - run
configure with `--enable-utempter` to enable this.

### From version control

To get and build the latest from version control - note that this requires
`autoconf`, `automake` and `pkg-config`:

~~~bash
git clone https://github.com/tmux/tmux.git
cd tmux
sh autogen.sh
./configure && make
~~~

## Contributing

Bug reports, feature suggestions and especially code contributions are most
welcome. Please send by email to:

tmux-users@googlegroups.com

Or open a GitHub issue or pull request. **Please read [this
document](CONTRIBUTING.md) before opening an issue.**

There is [a list of suggestions for contributions](https://github.com/tmux/tmux/wiki/Contributing).
Please feel free to ask on the mailing list if you're thinking of working on something or need
further information.

## Documentation

For documentation on using tmux, see the tmux.1 manpage. View it from the
source tree with:

~~~bash
nroff -mdoc tmux.1|less
~~~

A small example configuration is in `example_tmux.conf`.

And a bash(1) completion file at:

https://github.com/imomaliev/tmux-bash-completion

For debugging, run tmux with `-v` or `-vv` to generate server and client log 
files in the current directory.

## Support

The tmux mailing list for general discussion and bug reports is:

https://groups.google.com/forum/#!forum/tmux-users

Subscribe by sending an email to:

tmux-users+subscribe@googlegroups.com

<!-- 外部リンク一覧 -->

[TMUX]:http://tmux.github.io/
[EAWA]:http://www.unicode.org/reports/tr11/#Ambiguous
[TMRP]:https://github.com/tmux/tmux.git
[KOIE]:https://github.com/koie
[PANE]:https://github.com/koie/tmux/commit/ac6c53ffd6c2987a3a4a5807df7fc6cca5d6ce88
[DIFF]:https://github.com/z80oolong/tmux-eaw-fix
[WCWD]:http://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c
[DRMK]:http://www.cl.cam.ac.uk/~mgk25/
[NICM]:https://github.com/nicm
[ZOOL]:http://zool.jpn.org/
[ISCL]:https://www.isc.org/downloads/software-support-policy/isc-license/
[READ]:https://github.com/tmux/tmux/blob/master/.github/README.md
