#include "system.h"
#include <QMediaPlayer>
#include <QAudioOutput>
#include <vulkan/vulkan.h>
#include <cpuid.h>
void playMatchFoundSound(const QString& file) {
    static QMediaPlayer* player = [] { auto* p = new QMediaPlayer(qApp); p->setAudioOutput(new QAudioOutput(p));
        QObject::connect(p, &QMediaPlayer::errorOccurred, p, [p](QMediaPlayer::Error, const QString&) {
            const QUrl fallback("qrc:/sounds/match_found.wav");
            if(p->source()!=fallback) { p->setSource(fallback); p->play(); }
        }); return p; }();
    player->stop();
    player->setSource(file.isEmpty() || !QFileInfo::exists(file) ? QUrl("qrc:/sounds/match_found.wav") : QUrl::fromLocalFile(file));
    player->play();
}
QString cpuMissing() {
    unsigned a,b,c,d; QStringList missing;
    if (!__get_cpuid(1, &a,&b,&c,&d)) return "x86-64-v2";
    for (auto [bit,name] : {std::pair{0,"SSE3"},{9,"SSSE3"},{13,"CMPXCHG16B"},{19,"SSE4.1"},{20,"SSE4.2"},{23,"POPCNT"}})
        if (!(c & (1u << bit))) missing << name;
    return missing.join(", ");
}
QList<Gpu> gpuList() {
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO}; app.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO}; ci.pApplicationInfo = &app;
    VkInstance instance; QList<Gpu> out;
    if (vkCreateInstance(&ci, nullptr, &instance) != VK_SUCCESS) return out;
    uint32_t n = 0;
    if (vkEnumeratePhysicalDevices(instance, &n, nullptr) == VK_SUCCESS) {
        std::vector<VkPhysicalDevice> devices(n);
        if (vkEnumeratePhysicalDevices(instance, &n, devices.data()) == VK_SUCCESS)
            for (uint32_t i = 0; i < n; ++i) {
                VkPhysicalDeviceProperties p; vkGetPhysicalDeviceProperties(devices[i], &p);
                out.push_back({int(i), QString::fromUtf8(p.deviceName)});
            }
    }
    vkDestroyInstance(instance, nullptr); return out;
}
QString autoGpu() { return gpuList().value(0).name; }
QString machineId() {
    auto id = QSysInfo::machineUniqueId();
    return id.isEmpty() ? QString() : QString::fromLatin1(QCryptographicHash::hash("t6:" + id, QCryptographicHash::Sha256).toHex().left(32));
}
QString guessRegion()
{
	const QString tz = QString::fromLatin1(QTimeZone::systemTimeZoneId());
	const auto in = [&tz](std::initializer_list<const char*> zones) {
		for (const char* z : zones)
			if (tz.startsWith(z))
				return true;
		return false;
	};
	if (in({"America/Sao_Paulo", "America/Argentina", "America/Buenos_Aires", "America/Santiago", "America/Bogota", "America/Lima",
	        "America/Caracas", "America/Montevideo", "America/Asuncion", "America/La_Paz", "America/Guayaquil", "America/Manaus",
	        "America/Bahia", "America/Fortaleza", "America/Recife", "America/Belem", "America/Cayenne", "America/Paramaribo",
	        "America/Guyana"}))
		return "SA";
	if (in({"Asia/Dubai", "Asia/Riyadh", "Asia/Qatar", "Asia/Kuwait", "Asia/Bahrain", "Asia/Muscat", "Asia/Aden", "Asia/Tehran",
	        "Asia/Baghdad", "Asia/Jerusalem", "Asia/Tel_Aviv", "Asia/Gaza", "Asia/Hebron", "Asia/Amman", "Asia/Beirut", "Asia/Damascus"}))
		return "ME";
	if (in({"Asia/Singapore", "Asia/Kuala_Lumpur", "Asia/Kuching", "Asia/Bangkok", "Asia/Jakarta", "Asia/Pontianak", "Asia/Makassar",
	        "Asia/Jayapura", "Asia/Manila", "Asia/Ho_Chi_Minh", "Asia/Saigon", "Asia/Phnom_Penh", "Asia/Vientiane", "Asia/Yangon",
	        "Asia/Rangoon", "Asia/Brunei", "Asia/Dili"}))
		return "SEA";
	return in({"Europe/", "Atlantic/"})          ? "EU"
	       : in({"America/", "US/", "Canada/"})  ? "USA"
	       : in({"Asia/", "Indian/"})            ? "ASIA"
	       : in({"Australia/", "Pacific/"})      ? "OCE"
	       : in({"Africa/"})                     ? "AF"
	                                             : "EU";
}

QString localIp()
{
	for (const auto& iface : QNetworkInterface::allInterfaces())
	{
		if (!(iface.flags() & QNetworkInterface::IsUp) || (iface.flags() & QNetworkInterface::IsLoopBack))
			continue;
		for (const auto& e : iface.addressEntries())
			if (e.ip().protocol() == QAbstractSocket::IPv4Protocol && !e.ip().isLinkLocal())
				return e.ip().toString();
	}
	return "127.0.0.1";
}
