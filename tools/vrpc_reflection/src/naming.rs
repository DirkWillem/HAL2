use regex::Regex;

pub fn upper_snake_case_to_pascal_case(src: &str) -> String {
    src.split("_")
        .map(|v| {
            let ch0 = &v[0..1];
            let ch_rest = &v[1..];
            return format!("{}{}", ch0, ch_rest.to_lowercase());
        })
        .collect::<Vec<_>>()
        .join("")
}

pub fn pascal_case_to_upper_snake_case(src: &str) -> String {
    let upcase_re = Regex::new("([A-Z])").unwrap();

    upcase_re
        .replace_all(src, "_$0")
        .trim_start_matches("_")
        .to_uppercase()
}

pub fn pascal_case_to_lower_snake_case(src: &str) -> String {
    let upcase_re = Regex::new("([A-Z])").unwrap();

    upcase_re
        .replace_all(src, "_$0")
        .trim_start_matches("_")
        .to_lowercase()
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn test_upper_snake_case_to_pascal_case() {
        assert_eq!(upper_snake_case_to_pascal_case("FOO"), "Foo");
        assert_eq!(upper_snake_case_to_pascal_case("FOO_BAR"), "FooBar");
        assert_eq!(
            upper_snake_case_to_pascal_case("FOO_BAR_BAZ_QUUX"),
            "FooBarBazQuux"
        );
    }

    #[test]
    fn test_pascal_case_to_upper_snake_case() {
        assert_eq!(pascal_case_to_upper_snake_case("Foo"), "FOO");
        assert_eq!(pascal_case_to_upper_snake_case("FooBar"), "FOO_BAR");
        assert_eq!(
            pascal_case_to_upper_snake_case("FooBarBazQuux"),
            "FOO_BAR_BAZ_QUUX"
        );
    }

    #[test]
    fn test_pascal_case_to_lower_snake_case() {
        assert_eq!(pascal_case_to_lower_snake_case("Foo"), "foo");
        assert_eq!(pascal_case_to_lower_snake_case("FooBar"), "foo_bar");
        assert_eq!(
            pascal_case_to_lower_snake_case("FooBarBazQuux"),
            "foo_bar_baz_quux"
        );
    }
}
