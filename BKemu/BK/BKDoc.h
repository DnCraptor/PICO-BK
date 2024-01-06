
// BKDoc.h : интерфейс класса CBKDoc
//


#pragma once


class CBKDoc : public CDocument
{
	protected: // создать только из сериализации
		CBKDoc();
		DECLARE_DYNCREATE(CBKDoc)

// Атрибуты
	public:

// Операции
	public:

// Переопределение
	public:
		virtual BOOL OnNewDocument() override;
		virtual void Serialize(CArchive &ar) override;
#ifdef SHARED_HANDLERS
		virtual void InitializeSearchContent() override;
		virtual void OnDrawThumbnail(CDC &dc, LPRECT lprcBounds) override;
#endif // SHARED_HANDLERS

// Реализация
	public:
		virtual ~CBKDoc() override;
#ifdef _DEBUG
		virtual void AssertValid() const override;
		virtual void Dump(CDumpContext &dc) const override;
#endif

// Созданные функции схемы сообщений
	protected:
		virtual void SetTitle(LPCTSTR lpszTitle) override;
		DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
		// Вспомогательная функция, задающая содержимое поиска для обработчика поиска
		void SetSearchContent(const CString &value);
#endif // SHARED_HANDLERS

};
