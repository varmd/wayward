pkgname=wayward
pkgver=0.9.0
pkgrel=1
pkgdesc="wayward - fast desktop shell for wayland and weston"
arch=('x86_64')
url="https://github.com/varmd/wayward"

license=('GPL')
provides=("wayward")
depends=("weston" "gtk3" "adwaita-icon-theme" "ttf-droid")

makedepends=( "fakeroot" "gcc")

export _source="$PWD"
prepare() {
  cd "$srcdir/"
  rm -rf $srcdir/source
  cp -r "$_source"/source .
}

build() {
	cd "$srcdir/source"
	sh build.sh
}

package() {
	cd "$srcdir/source"

	sh install.sh $pkgdir
  rm -rf $srcdir
}
