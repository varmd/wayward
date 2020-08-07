pkgname=wayward
pkgver=0.7.1
pkgrel=3
pkgdesc="wayward - fast shell for wayland and weston"
arch=('x86_64')
url="https://github.com/varmd/wayward"

license=('GPL')
provides=("wayward")
depends=("weston" "gtk3")

makedepends=( "fakeroot" "gcc")
install=wayward.install


prepare() {
  cd "$srcdir/"
  cp -r ../source .
}

build() {
	cd "$srcdir/source"
	sh build.sh
}

package() {
	cd "$srcdir/source"

	sh install.sh $pkgdir
          
}
