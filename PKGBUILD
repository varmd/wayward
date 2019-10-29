pkgname=wayward
pkgver=r6
pkgrel=2
pkgdesc="Lightweight desktop environment for Weston compositor and Wayland"
arch=('x86_64')
url="https://github.com/raspberrypi/wayward"
license=('custom:MIT' 'GPL')
provides=("wayward")
depends=("weston")

makedepends=( "fakeroot" "gcc")
install=wayward.install
#source=("")

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
