pkgname=wayward
pkgver=r6
pkgrel=12
pkgdesc="Lightweight desktop environment based on Weston compositor and GTK on  Wayland"
arch=('x86_64')
url="https://github.com/varmd/wayward"

license=('GPL')
provides=("wayward")
depends=("weston")

makedepends=( "fakeroot" "gcc")
install=wayward.install


prepare() {
	cd "$srcdir/"
}

build() {
	cd "$srcdir/"
	sh build.sh
}

package() {
	cd "$srcdir/"
        echo $pkgdir
	sh install.sh $pkgdir
          
}
