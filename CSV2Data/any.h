#ifndef ANY_H
#define ANY_H

#define IVEN_ANY 

#include <typeinfo>
#include <sstream>
#include <string>
#include <vector>
namespace any_type{
	class any
	{
	public:
		//class placeholder
		class placeholder
		{
		public:
			virtual ~placeholder()
			{
			}

			virtual const std::type_info& type() const = 0;
			virtual placeholder* clone() const = 0;
		};

		//class holder
		template<typename ValueType>
		class holder : public placeholder
		{
		public:
			holder(const ValueType & value) : held(value)
			{
			}

			virtual const std::type_info& type() const
			{
				return typeid(ValueType);
			}

			virtual placeholder * clone() const
			{
				return new holder(held);
			}

			ValueType held;
		};


		template <class T>
		any(const T& a_right) :content_obj(new holder<T>(a_right))
		{
		}

		any(any const& a_right) : content_obj(a_right.content_obj ? a_right.content_obj->clone() : 0)
		{
		}

		template <typename T>
		any & operator = (T const& a_right)
		{
			if (content_obj) delete content_obj;
			content_obj = new T(a_right);
			return *this;
		}

		const std::type_info& type() const {
			return content_obj ? content_obj->type() : typeid(void);
		}

	private:
		template<typename ValueType>
		friend ValueType* any_cast(any*);

		placeholder * content_obj;
		int id;
	};

	template<typename ValueType>
	ValueType* any_cast(any* operand)
	{
		if (operand->content_obj && typeid(ValueType) == operand->type())
		{
			return &static_cast<any::holder<ValueType>*>(operand->content_obj)->held;
		}
		else return nullptr;
	}

	template<typename ValueType>
	const ValueType* any_cast(const any* operand)
	{
		return any_cast<ValueType>(const_cast<any*>(operand));
	}

	template<typename ValueType>
	ValueType& any_cast(any& operand)
	{
		ValueType* ptr = any_cast<ValueType>(&operand);
		if (ptr) return *ptr;
		else
		{
			stringstream err;
			err << "Cannot cast " << operand.type().name() << " to " << typeid(ValueType).name() << endl;
			const string& errstr = err.str();
			throw(bad_typeid(errstr.c_str()));
		}
	}

	template<typename ValueType>
	const ValueType& any_cast(const any& operand)
	{
		const ValueType* ptr = any_cast<ValueType>(const_cast<any*>(&operand));
		if (ptr) return *ptr;
		else
		{
			stringstream err;
			err << "Cannot cast " << operand.type().name() << " to " << typeid(ValueType).name() << endl;
			const string& errstr = err.str();
			throw(bad_typeid(errstr.c_str()));
		}
	}
}//namespace any_type
#endif //ANY_H 