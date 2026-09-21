#include "abntpiano/App.hpp"

int main(int, char*[]) {
    abntpiano::App app;
    if (!app.init()) {
        return 1;
    }
    app.run();
    return 0;
}
