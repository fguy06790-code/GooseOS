// LICENSE: GPL-3.0-or-later
// Exercise the Goose implementation directly without native adapters.
import catalog_retry from "../../goose";

fn main() {
    let transient: u32[7] = [1231, 2250, 12002, 12007, 12029, 12030, 12031];
    for code in transient {
        assert(catalog::thyfos_catalog_retryable(code));
    }

    // Success, access/format/hash failures and certificate failures must not
    // receive a retry. Include adjacent codes to catch widened ranges.
    let permanent: u32[19] = [
        0, 5, 11, 23, 13, 12037, 12045,
        1230, 1232, 2249, 2251, 12001, 12003, 12006, 12008,
        12028, 12032, 12175, 4294967295
    ];
    for code in permanent {
        assert(!catalog::thyfos_catalog_retryable(code));
    }
    print("Goose catalog retry policy: transient failures accepted; trust, format and access failures excluded");
}
