// Tekken 6 Native Online launcher - Controllers and keyboard: buttons, bindings, key names, the controller panel, key capture.

#include "controls.h"
#include "files.h"

	// combination of the controller's Button layout - 1+3, 2+4, 2+3, 1+4 - as well)

int kbKey(const QString& key, int def)
{
	return settings().value("kb_" + key, def).toInt();
}

QString vkName(int vk)
{
	if (!vk)
		return QString::fromUtf8("–");
#ifdef _WIN32
	UINT sc = MapVirtualKeyW(UINT(vk), MAPVK_VK_TO_VSC);
	switch (vk)  // extended keys: the name of the gray block, not of the numpad
	{
	case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN: case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
	case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK: case VK_RCONTROL: case VK_RMENU:
		sc |= 0x100;
		break;
	default:
		break;
	}
	wchar_t name[64]{};
	if (GetKeyNameTextW(static_cast<LONG>(sc) << 16, name, 64) > 0)
		return QString::fromWCharArray(name);
	return QString("VK %1").arg(vk);
#else
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9') || vk==32) return QString(QChar(vk));
    if(vk>=96 && vk<=105) return QString("Numpad %1").arg(vk-96);
    if(vk>=112 && vk<=135) return QString("F%1").arg(vk-111);
    static const QMap<int,QString> names{{8,"Backspace"},{9,"Tab"},{13,"Enter"},{16,"Shift"},{17,"Control"},{18,"Alt"},{20,"Caps Lock"},{27,"Escape"},{37,"Left"},{38,"Up"},{39,"Right"},{40,"Down"},
        {186,";"},{187,"="},{188,","},{189,"-"},{190,"."},{191,"/"},{192,"`"},{219,"["},{220,"\\"},{221,"]"},{222,"'"}};
    return names.value(vk, QString("Key %1").arg(vk));
#endif
}

// T6_KEYMAP of the game for the Custom layout
QString keyMap()
{
	QStringList parts;
	for (const KbTarget& t : kKbTargets)
		if (const int vk = kbKey(t.key, t.def))
			parts << QString("%1:%2").arg(vk, 0, 16).arg(t.bits, 0, 16);
	return parts.join(',');
}

// WinMM joysticks that are connected: (index, name)
QList<QPair<int, QString>> mmDevices()
{
	QList<QPair<int, QString>> out;
	for (UINT i = 0; i < joyGetNumDevs() && i < 16; i++)
	{
		JOYINFOEX ji{sizeof(ji), JOY_RETURNALL};
		JOYCAPSW caps{};
		if (joyGetPosEx(i, &ji) == JOYERR_NOERROR && joyGetDevCapsW(i, &caps, sizeof(caps)) == JOYERR_NOERROR)
			out.push_back({int(i), QString::fromWCharArray(caps.szPname)});
	}
	return out;
}

PadPanel::PadPanel() { setFixedSize(980, 470); }

// key: the MmTarget / PadButton key (case does not matter); false when the pad has no place for it
bool PadPanel::add(const QString& key, const QString& label, QWidget* field)
{
	static const QHash<QString, QPoint> at = {
		{"LT", {110, 0}}, {"LB", {110, 75}}, {"RT", {710, 0}}, {"RB", {710, 75}},
		{"BACK", {320, 90}}, {"START", {500, 90}},
		{"UP", {110, 160}}, {"LEFT", {0, 235}}, {"RIGHT", {220, 235}}, {"DOWN", {110, 310}},
		{"Y", {710, 160}}, {"X", {600, 235}}, {"B", {820, 235}}, {"A", {710, 310}},
		{"LS", {320, 320}}, {"RS", {500, 320}}};
	const auto it = at.find(key.toUpper());
	if (it == at.end())
		return false;
	auto* l = new QLabel(label.toUpper(), this);
	l->setObjectName("ddLabel");
	l->move(*it);
	field->setParent(this);
	field->setFixedWidth(kField);
	field->move(it->x(), it->y() + 20);
	return true;
}

void PadPanel::paintEvent(QPaintEvent*)
{
	// a rounded body (shoulder buttons on its top edge, the sticks inside at the bottom) and two rounded grips
	QPainterPath body, grips;
	body.addRoundedRect(QRectF(40, 110, 900, 295), 110, 110);
	grips.addRoundedRect(QRectF(40, 260, 290, 205), 100, 100);
	grips.addRoundedRect(QRectF(650, 260, 290, 205), 100, 100);
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	p.setPen(QPen(QColor(255, 255, 255, 46), 2));
	p.setBrush(QColor(255, 255, 255, 10));
	p.drawPath(body.united(grips));
}

KeyGrab::KeyGrab(QObject* parent, std::function<void(int)> done) : QObject(parent), m_done(std::move(done)) {}

bool KeyGrab::eventFilter(QObject*, QEvent* e)
{
	if (e->type() != QEvent::KeyPress || !m_done)
		return false;
	#ifdef _WIN32
    const int vk = int(static_cast<QKeyEvent*>(e)->nativeVirtualKey());
#else
    auto* keyEvent = static_cast<QKeyEvent*>(e);
    const int q = keyEvent->key();
    static const QMap<int,int> special{{Qt::Key_Return,13},{Qt::Key_Enter,13},{Qt::Key_Backspace,8},{Qt::Key_Tab,9},
        {Qt::Key_Escape,27},{Qt::Key_Left,37},{Qt::Key_Up,38},{Qt::Key_Right,39},{Qt::Key_Down,40},
        {Qt::Key_Home,36},{Qt::Key_End,35},{Qt::Key_PageUp,33},{Qt::Key_PageDown,34},{Qt::Key_Insert,45},{Qt::Key_Delete,46},
        {Qt::Key_Shift,16},{Qt::Key_Control,17},{Qt::Key_Alt,18},{Qt::Key_CapsLock,20},
        {Qt::Key_Semicolon,186},{Qt::Key_Equal,187},{Qt::Key_Comma,188},{Qt::Key_Minus,189},{Qt::Key_Period,190},
        {Qt::Key_Slash,191},{Qt::Key_QuoteLeft,192},{Qt::Key_BracketLeft,219},{Qt::Key_Backslash,220},{Qt::Key_BracketRight,221},{Qt::Key_Apostrophe,222}};
    int vk=special.value(q, q>=32 && q<=126 ? q : 0);
    if(q>=Qt::Key_F1 && q<=Qt::Key_F24) vk=112+q-Qt::Key_F1;
    if(keyEvent->modifiers() & Qt::KeypadModifier) {
        if(q>=Qt::Key_0 && q<=Qt::Key_9) vk=96+q-Qt::Key_0;
        if(q==Qt::Key_Plus) vk=107;
        if(q==Qt::Key_Minus) vk=109;
        if(q==Qt::Key_Asterisk) vk=106;
        if(q==Qt::Key_Slash) vk=111;
        if(q==Qt::Key_Period) vk=110;
    }
#endif
	auto done = std::move(m_done);
	m_done = nullptr;
	done(vk);
	return true;
}
